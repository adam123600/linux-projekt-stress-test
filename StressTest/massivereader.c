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


#define MAX_EPOLL_EVENTS 100


struct typPolaczeniaStruct{
     
    // int fd;
    // int type;                       // 1-server     2-inet      3-local
    // struct sockaddr_un address;
    int fileDescriptor;
    int typPolaczenia; // 1-server     2-inet      3-local
    struct sockaddr_un addressUn;
};

int server_fd; // deskryptor do servera
int epoll_fd; // deskryptor do epoll
int socketDoPolaczeniaDoKlienta;
int newsockfd; // deskryptor socketa od klienta
int logFileDescriptor; // deskryptor do pliku z logami
int liczbaPlikowOtwartychLogi;

struct sockaddr_in address;
struct epoll_event ev; // struktura do epolla
struct epoll_event evTemp; // struktura do funkcji: dodanie_do_epoll
struct epoll_event events[MAX_EPOLL_EVENTS];

void tworzenie_serwer(int port);
void tworzenie_epoll();
void dodanie_do_epoll(struct typPolaczeniaStruct* strukturaTypPolaczenia, int typDoEpoll);
void czytanieParametrow(int argc, char** argv, int* port, char** prefiksSciezki);
void czytanieStruktury(int fileDescriptor);
void nonBlock(int fileDescriptor);
void nasluchiwanieServer(int fileDescriptor);
void akceptowaniePolaczenia(int fileDescriptor);
void polaczenieJakoKlient(int* socketTemp, int* port);
void probaPolaczenia(int fileDescriptor);
int polaczenieKlientLokal(struct sockaddr_un* sockaddrUN);
char* reprezentacjaTekstowaCzasu (struct timespec strukturaCzas);
void utworzenieNowegoPlikuLogi(char* prefiksSciezki);
void wpisywanieDoPlikuLogi(struct typPolaczeniaStruct* typPolaczeniaStruct);
void roznicaCzasu(struct timespec* czasStart,
         struct timespec* czasKoniec, struct timespec* czasRoznicaStruktura);


int main (int argc, char** argv)
{
    int sockfd;
    int newsockfd = 0;
    int portno;
    socklen_t clilen;
    char buffer[256];
    int n;
    char* prefiksSciezki = NULL;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    int port;

    /////////////////////////////////////////////////////////
    if ((epoll_fd = epoll_create1(0)) == -1)
    {
        printf("Blad tworzenie epoll main\n");
        exit(-1);
    }
    czytanieParametrow(argc, argv, &port, &prefiksSciezki);
    tworzenie_serwer(port);
    utworzenieNowegoPlikuLogi(prefiksSciezki);

    while(1)
    {
        int nfds = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);
        
        if(nfds == -1)
        {
            printf("Blad nfds, main while(1) - massivereader\n");
            exit(-1);
        }


        for(int i = 0; i < nfds; i++)
        {
           // printf("for count : %d\n", i);
         //   printf("Typ polacznia: %d\n",(((struct typPolaczeniaStruct*)events[i].data.ptr)->typPolaczenia) );

             if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP || !(events[i].events & EPOLLIN))
            {
                printf("\n\nEVENTS FLAGI MASSIVEREADER\n\n");


                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                //if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL) == -1 )
                // {
                //     printf("Blad epoll_ctl while(1) main- masivereader\n");
                //     exit(-1);
                // }

                //if ( close((struct typPolaczeniaStruct*)events[i].data.ptr)->typPolaczenia == -1)
                // if(close (events[i].data.fd) == -1)
                // {
                //     printf("Cannot close file descriptor: %d", errno);
                //     exit(-1);
                // }
            }


            else if ((((struct typPolaczeniaStruct*)events[i].data.ptr)->typPolaczenia) == 1 )
            {
                /*
                 * akceptuje połączenie AF_INET
                 */

                printf("Jestem w server inet\n");
                /*int incomfd = */
                //acceptConnection((((struct typeOfConnection*)events[i].data.ptr)->fd), epoll_fd);
                akceptowaniePolaczenia(((struct typPolaczeniaStruct*)events[i].data.ptr)->fileDescriptor);
                printf("Serwer inet akceptuje polaczenie\n");

            }

            else if(((struct typPolaczeniaStruct*)events[i].data.ptr)->typPolaczenia == 2 )
            {
                /*
                * czytam strukture z polączenie AF_INET
                * tworze połaczenie AF_LOCAL
                * odsyłam strukturę przez AF_INET
                */
                printf("czytanie struktury i polaczenie af_local, odeslanie struktury\n");
                //czytanieStruktury(((struct typPolaczeniaStruct*)events[i].data.ptr)->fileDescriptor); // w tym podejmuje proby polaczenia
                probaPolaczenia(((struct typPolaczeniaStruct*)events[i].data.ptr)->fileDescriptor);
            }

            else
            {
               // printf("opcja 3 while1 main\n");
                //czytanieStruktury(((struct typPolaczeniaStruct*)events[i].data.ptr)->fileDescriptor); // w tym podejmuje proby polaczenia
                //probaPolaczenia(((struct typPolaczeniaStruct*)events[i].data.ptr)->fileDescriptor);
                
               // read(((struct typPolaczeniaStruct*)events[i].data.ptr)->fileDescriptor, buffer, 21);
               // printf("DESKRYPTOR NR %d\n", ((struct typPolaczeniaStruct*)events[i].data.ptr)->fileDescriptor);
               // write(1, buffer, 21);
               // printf("\n\n");
                // czytanie struktury ktora przyjdzie po localu
                wpisywanieDoPlikuLogi(events[i].data.ptr);
            }
        }
        //sleep(1);
    }


    close(newsockfd);
    close(sockfd);

    return 0;

}


void tworzenie_serwer(int port)
{
   
    if ( (server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0 )
    {
        printf("Blad - tworzenie serwera\n");
        exit(-2);
    }

    address.sin_family = AF_INET; 
    //address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    //inet_aton("127.0.0.1", (struct in_addr *) &address.sin_addr.s_addr);
    address.sin_port = htons( port ); 
    // dopisac memset address.sin_zero ( jest w kliencie )

    if ( bind(server_fd, (struct sockaddr *)&address, sizeof(address) ) < 0 )
    {
        printf("Blad bind - tworzenie serwera\n");
        exit(-3);
    }

    nonBlock(server_fd);

    struct typPolaczeniaStruct* s = (struct typPolaczeniaStruct*)calloc(1, sizeof(struct typPolaczeniaStruct));
    s->fileDescriptor = server_fd;
    s->typPolaczenia = 1; // 1- server
    dodanie_do_epoll(s, EPOLLIN | EPOLLET);

    nasluchiwanieServer(server_fd);

    // sprobowac free s ( nie wiem czy mozna :-) )
}

void czytanieStruktury(int fileDescriptor)
{
      
      // sprobowac wyslac 
        // jesli tak to wyslac
        // else
            // poslac do autora
            // z flaga w sun_family -1
      struct sockaddr_un myStructUN;
      read(fileDescriptor, &myStructUN, sizeof(myStructUN));
      
      write(1, &myStructUN, sizeof(myStructUN));

    //  probaPolaczenia(&myStructUN);
    probaPolaczenia(fileDescriptor);

}


//void dodanie_do_epoll(int fileDescriptor, int typDoEpoll)
//void dodanie_do_epoll(struct* typPolaczeniaStruct, int typDoEpoll)
void dodanie_do_epoll(struct typPolaczeniaStruct* strukturaTypPolaczenia, int typDoEpoll)
{
    //printf("dodawanie do epoll AAAA\n");

    struct epoll_event structEvent;

    //structEvent.data.fd = fileDescriptor;
    structEvent.data.ptr = strukturaTypPolaczenia;
    structEvent.events = typDoEpoll;

   //if ( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, evTemp.data.fd, &structEvent) == -1 )
   if ( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ((struct typPolaczeniaStruct*)structEvent.data.ptr)->fileDescriptor, &structEvent) == -1 )
   {
       printf("Blad dodanie_do_epoll - serwer\n");
       exit(-1);
   }
}

void czytanieParametrow(int argc, char** argv, int* port, char** prefiksSciezki)
{
    int opt;
    int prefiksSciezkiFlag = 0;

    if ( argc != 4)
    {
        printf("Liczba argumentow sie nie zgadza- serwer\n");
        exit(-1);
    }

    while((opt = getopt(argc, argv, "O:")) != -1)
    {
        switch(opt)
        {
            case 'O':
                *prefiksSciezki = optarg;
                prefiksSciezkiFlag = 1;
                break;
        }
    }

    if ( !prefiksSciezkiFlag )
    {
        printf("Brak podanego argumentu: prefiksSciezki\n");
        exit(-1);
    }

    if ( (optind+1) != argc )
    {
        printf("Za duzo/malo parametrow- czytanieParametrow\n");
        exit(-1);
    }



    *port = strtol(argv[optind], NULL, 10);

    if ( *port < 6000 || *port > 65535)
    {
        printf("Podany PORT jest nieprawidlowy!\n");
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

void nasluchiwanieServer(int fileDescriptor)
{
    if ( listen(server_fd, 50) < 0)
    {
        printf("Blad listen main \n");
        exit(-1);
    }
}

void akceptowaniePolaczenia(int fileDescriptor)
{
   // struct sockaddr address;
   struct sockaddr_in address;

    int addrlen = sizeof(address);
    int newsockfd;

    if (( newsockfd = accept(fileDescriptor, (struct sockaddr *)&address, (socklen_t*)&addrlen)) == -1)
    {
            printf("Blad accept\n");
            exit(-1);
    }

    nonBlock(newsockfd);

    struct typPolaczeniaStruct* s = (struct typPolaczeniaStruct*)calloc(1, sizeof(struct typPolaczeniaStruct));
    s->fileDescriptor = newsockfd;
    s->typPolaczenia = 2; // inet
    dodanie_do_epoll(s, EPOLLIN | EPOLLET);
}

//void probaPolaczenia(struct sockaddr_un* sockaddrStruct)
void probaPolaczenia(int fileDescriptor)
{

    printf("PROBA POLACZENIA!!!\n");

    while(1)
    {        
        struct sockaddr_un myStructUN;

        if((read(fileDescriptor, &myStructUN, sizeof(struct sockaddr_un))) != sizeof(struct sockaddr_un))
        {
           printf("Blad czytania struktury, probaPolaczenia massivereader\n");
           break;
        }

            printf("ASDASD\n");

        if ( polaczenieKlientLokal(&myStructUN) == -1)
        {    
            myStructUN.sun_family = -1;
            write(fileDescriptor, &myStructUN, sizeof(struct sockaddr_un));
            printf("Wyslalem strukture do INET     PROBA POLACZENIA\n");
        }
        
        else  
        {
            printf("Udalo sie polaczyc! ProbaPolaczenia\n");
            write(fileDescriptor, &myStructUN, sizeof(struct sockaddr_un));
            write(1, &myStructUN, sizeof(struct sockaddr_un));
        }
 

        //sleep(1);
    }

    printf("KONIEC PROBA POLACZENIA!!!\n");
}


void polaczenieJakoKlient(int* socketTemp, int* port)
{
    *socketTemp = socket(AF_INET, SOCK_STREAM, 0);

    // sprawdzenie czy socket dziala

    nonBlock(*socketTemp);

    // tworze strukture 
    struct sockaddr_in tempSockaddr;

    tempSockaddr.sin_family = AF_INET; 
    tempSockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    tempSockaddr.sin_port = htons( *port ); 


    connect(*socketTemp, (struct sockaddr*)&tempSockaddr, sizeof(tempSockaddr));
    // sprawdzic czy dziala
}


int polaczenieKlientLokal(struct sockaddr_un* sockaddrUN)
{

    int socketTemp;

    if ( (socketTemp = socket(AF_LOCAL, SOCK_STREAM, 0)) == -1 )
        {
            printf("Blad utworzenia socketa- probaPolaczenia\n");
            return (-1);
        }



    if (( connect(socketTemp, (struct sockaddr *)sockaddrUN, sizeof(struct sockaddr_un))) == -1)
    {
        printf("Blad connectu - polaczenieKlientLokal\n");
        return (-1);
    }

    //nonBlock(socketTemp);

    struct typPolaczeniaStruct* s = (struct typPolaczeniaStruct*)calloc(1, sizeof(struct typPolaczeniaStruct));
    s->fileDescriptor = socketTemp;
    s->typPolaczenia = 3; // local
    s->addressUn = *sockaddrUN;

    dodanie_do_epoll(s, EPOLLIN | EPOLLET);

    return 1;
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


void utworzenieNowegoPlikuLogi(char* prefiksSciezki)
{
    char tempSciezka[1024];
    // deskryptor pliku --> logFileDescriptor

    int nowyFd = -1;

    while(nowyFd == -1)
    {
        sprintf(tempSciezka, "%s%03d", prefiksSciezki, liczbaPlikowOtwartychLogi++);
        nowyFd = open(tempSciezka, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    }

    close(logFileDescriptor);
    logFileDescriptor = nowyFd;
    
}

void wpisywanieDoPlikuLogi(struct typPolaczeniaStruct* typPolaczeniaStruct)
{
    // 1)wyznaczenie opoznienia
    // 2)weryfikacja nadawcy
    // 3)utworzenie reprezentacji tekstwocyh znacznika czasu i opoznieja, zgodnie z 3

    char czasZMultiwriter[21];
    char sciezkaZMultiwriter[108];
    struct timespec czasZMultiwriterStruktura;
    struct timespec czasAktualny;
    struct timespec czasRoznica;

    char* buffer = NULL;

    if (read(typPolaczeniaStruct->fileDescriptor, &czasZMultiwriter, 21) != 21)
    {
        ///printf("BLAD ODCZYTANIA czasZmult\n");
        printf("Blad read1, wpysywanieDoPlikuLogi, massivereader\n");
        exit(-1);
    }

    if (read(typPolaczeniaStruct->fileDescriptor, &sciezkaZMultiwriter, 108) != 108)
    {
        printf("Blad read2, wpisywanieDoPlikuLogi, massivereader\n");
        exit(-1);
    }
    
    if (read(typPolaczeniaStruct->fileDescriptor, &czasZMultiwriterStruktura, sizeof(struct timespec)) != sizeof(struct timespec))
    {
        printf("Blad read3, wpisywanieDoPlikuLogi, massivereader\n");
        exit(-1);
    }

    if ( clock_gettime(CLOCK_REALTIME, &czasAktualny) == -1)
    {
        printf("Blad clock_gettime, wpyisywanieDoPlikuLogi,, massivereader\n");
        exit(-1);
    }

    roznicaCzasu(&czasZMultiwriterStruktura, &czasAktualny, &czasRoznica);

    
    if ( strcmp(typPolaczeniaStruct->addressUn.sun_path, sciezkaZMultiwriter))
    {
        return;
    }


    //buffer = reprezentacjaTekstowaCzasu(czasRoznica);
    buffer = reprezentacjaTekstowaCzasu(czasAktualny);

    //write(logFileDescriptor, "ASD\n", strlen("ASD\n"));

    if ( write(logFileDescriptor, buffer, 20) < 20)
    {
        printf("BLAD ASDASD\n");
        exit(-1);
    }

    
    if ( write(logFileDescriptor, " : ", strlen(" : ")) < strlen(" : "))
    {
        printf("BLAD ASDASD\n");
        exit(-1);
    }

    if ( write(logFileDescriptor, czasZMultiwriter, 20) < 20)
    {
        printf("BLAD ASDASD\n");
        exit(-1);
    }

    if ( write(logFileDescriptor, " : ", strlen(" : ")) < strlen(" : "))
    {
        printf("BLAD ASDASD\n");
        exit(-1);
    }
    
    buffer = reprezentacjaTekstowaCzasu(czasRoznica);

    if ( write(logFileDescriptor, buffer, 20) < 20)
    {
        printf("BLAD ASDASD\n");
        exit(-1);
    }

    write(logFileDescriptor, "\n", strlen("\n"));




    //write(1, czasZMultiwriter, 21);
    //write(1, "\n", 1);
    //write(1, sciezkaZMultiwriter, 108);
}

void roznicaCzasu(struct timespec* czasStart,
         struct timespec* czasKoniec, struct timespec* czasRoznicaStruktura)
         {
             long long start;
             long long koniec;
             long long roznica;

             memset(czasRoznicaStruktura, 0, sizeof(struct timespec));

             start =  czasStart->tv_sec * 1000000000 + czasStart->tv_nsec;
             koniec = czasKoniec->tv_sec * 1000000000 + czasKoniec->tv_nsec;

             roznica = koniec - start;

             czasRoznicaStruktura->tv_sec = roznica / 1000000000;
             
             czasRoznicaStruktura->tv_nsec = roznica % 1000000000;

            // if( roznica % 1000000000 < 0)
            //     czasRoznicaStruktura->tv_nsec = -(roznica % 1000000000);
                

            //  printf("SEK: %ld\nNANO: %ld\n", czasRoznicaStruktura->tv_sec,
            //     czasRoznicaStruktura->tv_nsec);
         }