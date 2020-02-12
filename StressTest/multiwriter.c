#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <sys/epoll.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/random.h>
#include <errno.h>

#define MAX_EPOLL_EVENTS 100 


int server_fd; // fd servera z AF_LOCAL;
int epoll_fd; // 
struct sockaddr_un address;
struct epoll_event events[MAX_EPOLL_EVENTS];

int liczbaZaakceptowanychPolaczen;
int liczbaOdrzuconychPolaczen;
struct timespec czasSuma;
int flagaPetlaWhile;
long long minimum;
long long maximum;

void czytanieParametrow(int argc, char** argv, int* sIloscPolaczen,
            int* pPort, float* dOdstepCzasowy, float* tCalkowityCzasPracy);


void tworzenie_serwer(struct sockaddr_un sockaddrStruct);
void nonBlock(int fileDescriptor);
void losoweDane(struct sockaddr_un* sockaddrStruct);
void polaczenieJakoKlient(int* socketTemp, int* port);
void dodanie_do_epoll(int fileDesc, int typDoEpoll);
void nasluchiwanieServer(int fileDescriptor, int iloscPolaczen);
void akceptowaniePolaczenia(int fileDescriptor, int** tablicaDeskryptorowLokal);
void czytanieStruktury(int fileDescriptor);
void wyslanieStrukturyNaServer(struct sockaddr_un* sockaddrUN, int fileDescriptor, int iloscPolaczen);
void utworzenieBudzikaINastawienie(float calkowityCzasPracy);
char* reprezentacjaTekstowaCzasu(struct timespec strukturaCzas);
void doWyslaniaPrzezLokal(int* tablicaDeskryptorowLokal, struct sockaddr_un sockaddrUN);
void roznicaCzasu(struct timespec czasStart, struct timespec czasKoniec);
void obslugaSygnaluUSR1();
void handlerUSR1();

int main(int argc, char** argv)
{
    int port;
    int iloscPolaczen;
    float odstepCzasowy;
    float calkowityCzasPracy;
    liczbaZaakceptowanychPolaczen = 0;
    liczbaOdrzuconychPolaczen = 0;
    flagaPetlaWhile = 1;
    minimum = 10000000000;
    maximum = 0;

    obslugaSygnaluUSR1();

    czytanieParametrow(argc, argv, &iloscPolaczen, &port,
         &odstepCzasowy, &calkowityCzasPracy);

    if ((epoll_fd = epoll_create1(0)) == -1)
    {
        printf("Blad tworzenie epoll main\n");
        exit(-1);
    }

    int* tablicaDeskryptorowLokal = (int*)calloc(iloscPolaczen, sizeof(int));
    int* tablicaDeskryptorowLokalWskaznik = tablicaDeskryptorowLokal;

    ////////////////////////////////////////////////////
    struct sockaddr_un mySockaddrUN;
    losoweDane(&mySockaddrUN);
    tworzenie_serwer(mySockaddrUN);
    nasluchiwanieServer(server_fd, iloscPolaczen);
    ///////////////////////////////////////////////////

    ///////////////////////////////////////////////////
    int socketClient = 0;
    polaczenieJakoKlient(&socketClient, &port);
    
    ///////////////////////////////////////////////////
    wyslanieStrukturyNaServer(&mySockaddrUN, socketClient, iloscPolaczen);


    
    while(liczbaOdrzuconychPolaczen + liczbaZaakceptowanychPolaczen < iloscPolaczen)
    {

        int nfds = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);
   
        for(int i = 0; i < nfds; i++)
        {
            if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP || !(events[i].events & EPOLLIN))
            {
                {
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL) == -1)
                    {
                        printf("Blad epoll_ctl_del, while(1) main multiwriter\n");
                        exit(-1);
                    }

                      for(int i = 0; i < iloscPolaczen; ++i)
                            if(tablicaDeskryptorowLokal[i] == events[i].data.fd)
                                tablicaDeskryptorowLokal[i] = 0;
				      close(events[i].data.fd);

                }
            }

            else
            {
                if (events[i].data.fd == server_fd)
                {
                    akceptowaniePolaczenia(events[i].data.fd, &tablicaDeskryptorowLokalWskaznik);
                }
                
                else if (events[i].data.fd == socketClient)
                {
                    czytanieStruktury(socketClient);
                }
            }
        }

    }  

    if ( epoll_ctl(epoll_fd, EPOLL_CTL_DEL, socketClient, NULL) == -1)
    {
        printf("Blad epoll_ctl, po while(1) main, multiwriter.c\n");
        exit(-1);
    }

    if ( close(socketClient) == -1)
    {
        printf("Blad close socketClient, main po while(1), multiwriter.c\n");
        exit(-1);
    }

    utworzenieBudzikaINastawienie(calkowityCzasPracy);  

    struct timespec tim1;
    tim1.tv_sec = (int) (odstepCzasowy / 1000000);
    tim1.tv_nsec = (long long) (odstepCzasowy*1000) % 1000000000;


    while(flagaPetlaWhile)
    {
        doWyslaniaPrzezLokal(tablicaDeskryptorowLokal, mySockaddrUN);
        nanosleep(&tim1, NULL);
    }

    printf("CZAS: \n %li %li\n", czasSuma.tv_sec, czasSuma.tv_nsec);
    printf("MINIMUM: %lld\tMAXIMUM: %lld\n", minimum, maximum);

    exit(1);

}
void czytanieParametrow(int argc, char** argv, int* sIloscPolaczen,
                         int* pPort, float* dOdstepCzasowy, float* tCalkowityCzasPracy)
{
    int opt;
    int SFlag = 0;
    int pFlag = 0;
    int dFlag = 0;
    int TFlag = 0;

    if ( argc != 9)
    {
        printf("Liczba argumentow sie nie zgadza - klient\n");
        exit(-1);
    }

    while((opt = getopt(argc, argv, "S:p:d:T:")) != -1)
    {
        switch(opt)
        {
            case 'S':
                    *sIloscPolaczen = strtol(optarg, NULL, 10);
                    SFlag = 1;
                    break;
            case 'p':
                    *pPort = strtol(optarg, NULL, 10);
                    pFlag = 1;
                    break;
            case 'd':
                    *dOdstepCzasowy = strtof(optarg, NULL);
                    dFlag = 1;
            case 'T':
                    *tCalkowityCzasPracy = strtof(optarg, NULL);
                    TFlag = 1;
                    break;

        }
    }

    if ( !SFlag || !pFlag || !dFlag || !TFlag )
    {
        printf("Nie podano odpowiednich parametrow:\n");
        printf("-S lub -p lub -d lub -T\n");
        exit(-1);
    }
}




void nonBlock(int fileDescriptor)
{
    int flagi;

    if ( ( flagi = fcntl(fileDescriptor, F_GETFL, 0)) == -1)
    {
        printf("Blad nonBlock, odczytywaie flag - serwer\n");
        exit(-1);
    }

    if ( fcntl(fileDescriptor, F_SETFL, flagi | O_NONBLOCK) == -1)
    {
       printf("Blad nonBlock, doklejanie flagi - serwer\n");
       exit(-1);
    } 

}

void losoweDane(struct sockaddr_un* sockaddrStruct)
{
    char* tempBuffer = (char*)calloc(107, sizeof(char));
    sockaddrStruct->sun_family = AF_LOCAL;

    getrandom(tempBuffer, 107, GRND_NONBLOCK);
    sockaddrStruct->sun_path[0] = '\0';
    strcpy(&sockaddrStruct->sun_path[1], tempBuffer);

    free(tempBuffer);
}

void polaczenieJakoKlient(int* socketTemp, int* port)
{
    if ((*socketTemp = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
    {
        // sprawdzenie czy socket dziala
        printf("Blad socket, polaczenieJakoKlient = multiwriter\n");
        exit(-1);
    }


    // tworze strukture 
    struct sockaddr_in tempSockaddr;

    tempSockaddr.sin_family = AF_INET; 
    tempSockaddr.sin_port = htons( *port ); 
    tempSockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");


    if (connect(*socketTemp, (struct sockaddr*)&tempSockaddr, sizeof(tempSockaddr)) == -1)
    {
        // sprawdzic czy dziala
        printf("Blad connect, polaczenieJakoKlient - multiwriter\n");
        exit(-1);
    }

    nonBlock(*socketTemp);

    dodanie_do_epoll(*socketTemp, EPOLLIN | EPOLLET);
}


void dodanie_do_epoll(int fileDescriptor, int typDoEpoll)
{
    //printf("dodawanie do epoll AAAA\n");

    struct epoll_event structEvent;

    structEvent.data.fd = fileDescriptor;
    structEvent.events = typDoEpoll;

   //if ( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, evTemp.data.fd, &structEvent) == -1 )
   if ( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fileDescriptor, &structEvent) == -1 )
   {
       printf("Blad dodanie_do_epoll - serwer\n");
       exit(-1);
   }
}


void tworzenie_serwer(struct sockaddr_un sockaddrStruct)
{
    //server_fd
    if ((server_fd = socket(AF_LOCAL, SOCK_STREAM, 0)) == -1 )
    {
        printf("Blad socket - tworzenie server multiwriter\n");
        exit(-1);
    }

    if ( bind(server_fd, (struct sockaddr*)&sockaddrStruct, sizeof(struct sockaddr_un)) == -1)
    {
        printf("Blad bind - tworzenie server multiwriter\n");
        exit(-1);
    }

    nonBlock(server_fd);

    dodanie_do_epoll(server_fd, EPOLLIN | EPOLLET);

}


void nasluchiwanieServer(int fileDescriptor, int iloscPolaczen)
{
    if ( listen(server_fd, iloscPolaczen) < 0)
    {
        printf("Blad listen main \n");
        exit(-1);
    }
}


void akceptowaniePolaczenia(int fileDescriptor, int** tablicaDeskryptorowLokal)
{
    printf("AKCEPTOWANIE POLACZENIA!!!!!!!!!!!!!\n");

    int tempFileDescriptor = 0;

    if (( tempFileDescriptor = accept(server_fd, NULL, NULL)) == -1)
    {
        printf("Blad accept, akceptowaniePolaczenia - multiwriter\n");
        exit(-1);
    }

    //nonBlock(tempFileDescriptor);

    **tablicaDeskryptorowLokal = tempFileDescriptor;
    printf("TABLICA DESKRYPTOROW: %d\n", **tablicaDeskryptorowLokal);
    (*tablicaDeskryptorowLokal) += 1;
    dodanie_do_epoll(tempFileDescriptor, EPOLLIN | EPOLLET);
}
    
void czytanieStruktury(int fileDescriptor)
{
      
      // sprobowac wyslac 
        // jesli tak to wyslac
        // else
            // poslac do autora
            // z flaga w sun_family -1

    while(1)
    {
      struct sockaddr_un myStructUN;

     if ((read(fileDescriptor, &myStructUN,
         sizeof(struct sockaddr_un))) != sizeof(struct sockaddr_un))
      {
        break;
      }

      if ( myStructUN.sun_family >= 65535)
      {
        printf("Przyszla struktura ze statusem sun_family = -1\n");
        liczbaOdrzuconychPolaczen++;
      }
      
      else  
      {
          printf("Przyszla strutkura AF_LOCAL - czytanieStruktury = multiwriter\n");
          liczbaZaakceptowanychPolaczen++;
      }
    }
}

void wyslanieStrukturyNaServer(struct sockaddr_un* sockaddrUN, int fileDescriptor, int iloscPolaczen)
{
    for(int i = 0; i < iloscPolaczen; i++)
    {
       if ((write(fileDescriptor, sockaddrUN, sizeof(struct sockaddr_un))) == -1)
       {
           printf("Blad, wyslanieStrukturyNaServer - multiwriter\n");
           exit(-1);
       }
    }
    printf("Wyslano %d polaczen\n", iloscPolaczen);
}

void utworzenieBudzikaINastawienie(float calkowityCzasPracy)
{
    struct sigevent sev;
    struct itimerspec its;
    timer_t timerid;


    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGUSR1;
    sev.sigev_value.sival_ptr = &timerid;

    if ( timer_create(CLOCK_REALTIME, &sev, &timerid) == -1)
    {
        printf("Blad utworzenia budzika\n");
        exit(EXIT_FAILURE);
    }


    // zamiana z centy na nano
    double tempTime = calkowityCzasPracy * 10000000;

    its.it_value.tv_sec = (int) (tempTime / 1000000000);
    its.it_value.tv_nsec = (long long) tempTime % 1000000000;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    // its.it_value.tv_sec = 2;
    //  its.it_value.tv_nsec = 999999999;

    if ( timer_settime(timerid, 0, &its, NULL) == -1 )
    {
        printf("Blad timer_settime, utworzenieBudzikaINastawienie\n");
        printf("%d\n", errno);
        // errno 22 - wychodzi po za zakres 
        exit(-1);
    }
}

char* reprezentacjaTekstowaCzasu (struct timespec strukturaCzas)
{
    char* bufforWynikowy = (char*)calloc(21, sizeof(char));
    memset(bufforWynikowy, '0', 21);

    int iloscMinut;
    int iloscSekund;
    long long iloscNanosekund;

    iloscNanosekund = strukturaCzas.tv_nsec;

    iloscSekund = strukturaCzas.tv_sec % 60;
    iloscMinut = (strukturaCzas.tv_sec % 3600) / 60;

    if ( iloscMinut < 10 )
    {
        bufforWynikowy[2] = (char)iloscMinut + '0';
    }
    
    //bufforWynikowy[2] = (char)iloscMinut + '0';

    else  
    {
        bufforWynikowy[2] = (char)(iloscMinut % 10) + '0'; 
        bufforWynikowy[1] = (char)(iloscMinut / 10) + '0';
    }
    
    //write(1, bufforWynikowy, 3);

    bufforWynikowy[3] = '*';
    bufforWynikowy[4] = ':';

    if ( iloscSekund < 10 )
    {
        bufforWynikowy[6] = (char)iloscSekund + '0';
    }

    else
    {
        bufforWynikowy[6] = (char)(iloscSekund % 10) + '0';
        bufforWynikowy[5] = (char)(iloscSekund / 10) + '0';
    }


    // for(int i = 19; i > 16; i--)
    // {
    //     bufforWynikowy[i] = (char)(iloscNanosekund % (i*10)) + '0';
    // }

    bufforWynikowy[7] = ',';
    bufforWynikowy[8] = (char)(iloscNanosekund   /100000000   % 10) + '0';
    bufforWynikowy[9] = (char)(iloscNanosekund   /10000000    % 10) + '0';
    bufforWynikowy[10] = '.';
    bufforWynikowy[11] = (char)(iloscNanosekund  /1000000     % 10) + '0';
    bufforWynikowy[12] = (char)(iloscNanosekund  /100000      % 10) + '0';
    bufforWynikowy[13] = '.';
    bufforWynikowy[14] = (char)(iloscNanosekund  /10000       % 10) + '0';
    bufforWynikowy[15] = (char)(iloscNanosekund  /1000        % 10) + '0';
    bufforWynikowy[16] = '.';
    bufforWynikowy[17] = (char)(iloscNanosekund  /100         % 10) + '0';
    bufforWynikowy[18] = (char)(iloscNanosekund  /10          % 10) + '0';
    bufforWynikowy[19] = (char)(iloscNanosekund               % 10) + '0';

//    write(1, bufforWynikowy, 20);

    return bufforWynikowy;
}


void doWyslaniaPrzezLokal(int* tablicaDeskryptorowLokal, struct sockaddr_un sockaddrUN)
{
    srand(time(NULL));

    struct timespec czasStart;
    struct timespec czasKoniec;

    char* czasString = 0;

    if (clock_gettime(CLOCK_REALTIME, &czasStart) == -1)
    {
        printf("Blad clock_gettime czasStart, doWyslaniaPrzezLokal - multiwriter\n");
        exit(-1);
    }

    czasString = reprezentacjaTekstowaCzasu(czasStart);

    int losowyIndex = rand() % liczbaZaakceptowanychPolaczen;

    while(tablicaDeskryptorowLokal[losowyIndex] == 0)
    {
        losowyIndex = rand() % liczbaZaakceptowanychPolaczen;
    }

    // wyslac
    // 1) tekstowa reprezentacja czasu
    // 2) adres gniazda, ktory byl wyslany przy rejestracji
    // 3) liczbowa wartosc znacznika (struktura timespec)

    if (write(tablicaDeskryptorowLokal[losowyIndex], czasString, 21) < 21 )
    {
        printf("Blad write1 doWyslaniaPrzezLokal multiwriter\n");
        exit(-1);
    }

    if (write(tablicaDeskryptorowLokal[losowyIndex], &sockaddrUN.sun_path, 108) < 108)
    {
        printf("Blad write2 doWyslaniaPrzezLokal multiwriter\n");
        exit(-1);
    }

    if (write(tablicaDeskryptorowLokal[losowyIndex], &czasStart, sizeof(czasStart)) < sizeof(czasStart))
    {
        printf("Blad write3 doWyslaniaPrzezLokal multiwriter\n");
        exit(-1);
    }

    if (clock_gettime(CLOCK_REALTIME, &czasKoniec) == -1)
    {
        printf("Blad clock_gettime czasKoniec, doWyslaniaPrzezLokal - multiwriter\n");
        exit(-1);
    }

    // roznica czasu
    roznicaCzasu(czasStart, czasKoniec);
}


void roznicaCzasu(struct timespec czasStart, struct timespec czasKoniec)
{
    // czasStart.tv_sec += czasKoniec.tv_sec - czasStart.tv_sec + ((czasSuma.tv_nsec + czasKoniec.tv_nsec - czasStart.tv_nsec)/1000000000);
    // long temp = (czasSuma.tv_nsec + czasKoniec.tv_nsec - czasStart.tv_nsec)%1000000000;

    // if ( temp < 0)
    //     czasSuma.tv_nsec = -temp;
    // else 
    // {
    //     czasSuma.tv_nsec = temp;
    // }

    //     if( a->tv_nsec + b->tv_nsec >= (long)1e9 )
    // {
    //     wynik->tv_sec = a->tv_sec + b->tv_sec + 1;
    //     wynik->tv_nsec= a->tv_nsec + b->tv_nsec - (long)1e9;
    // }
    // else
    // {
    //     wynik->tv_sec = a->tv_sec + b->tv_sec;
    //     wynik->tv_nsec = a->tv_nsec + b->tv_nsec;
    // }


    long long nanoSekundy;
    long long sekundy;

    nanoSekundy = czasKoniec.tv_nsec - czasStart.tv_nsec;
    sekundy = czasKoniec.tv_sec - czasStart.tv_sec;

    if ( nanoSekundy < 0)
    {
        nanoSekundy = 1000000000 + nanoSekundy;
        sekundy--;
    }

    czasSuma.tv_sec += sekundy;

    if ((czasSuma.tv_nsec + nanoSekundy) > 999999999)
    {
        czasSuma.tv_nsec = (czasSuma.tv_nsec + nanoSekundy) % 1000000000;
        czasSuma.tv_sec++;
    }

    else
    {
        czasSuma.tv_nsec += nanoSekundy;
    }

    if ( nanoSekundy < minimum)
    {
        minimum = nanoSekundy;
    }
    if ( nanoSekundy > maximum)
    {
        maximum = nanoSekundy;
    }

    
}

void obslugaSygnaluUSR1()
{
    struct sigaction sa;
    sa.sa_flags = 0;
    sa.sa_handler = handlerUSR1;

    if ( sigaction(SIGUSR1, &sa, NULL) == -1)
    {
        printf("Blad sigaction- obslugaSygnaluUSR1, multiwriter\n");
        exit(-1);
    }
}

void handlerUSR1()
{
    flagaPetlaWhile = 0;
}