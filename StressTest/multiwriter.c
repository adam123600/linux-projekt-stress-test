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

#define MAX_EPOLL_EVENTS 100 


int server_fd; // fd servera z AF_LOCAL;
int epoll_fd; // 
//struct sockaddr_in address;
struct sockaddr_un address;
struct epoll_event events[MAX_EPOLL_EVENTS];

int liczbaZaakceptowanychPolaczen;
int liczbaOdrzuconychPolaczen;


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



int main(int argc, char** argv)
{
    int port;
    int iloscPolaczen;
    float odstepCzasowy;
    float calkowityCzasPracy;
    liczbaZaakceptowanychPolaczen = 0;
    liczbaOdrzuconychPolaczen = 0;

    czytanieParametrow(argc, argv, &iloscPolaczen, &port,
         &odstepCzasowy, &calkowityCzasPracy);
/////////////////////////////////////////////////////////
    
    if ((epoll_fd = epoll_create1(0)) == -1)
    {
        printf("Blad tworzenie epoll main\n");
        exit(-1);
    }

    int* tablicaDeskryptorowLokal = (int*)calloc(iloscPolaczen, sizeof(int));

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

    // utworzenie socketu
    // int mySocket = socket(AF_INET, SOCK_STREAM, 0);

    // struct sockaddr_in cli_addr; // struktura do socketa- klient

    // cli_addr.sin_port = htons(port);
    // cli_addr.sin_family = AF_INET;
    // cli_addr.sin_addr.s_addr = inet_addr("127.0.0.1");


    

    // if ( connect(mySocket, (struct sockaddr*)&cli_addr, sizeof(cli_addr)) == -1)
    // {
    //     printf("Nie udalo sie polaczyc\n");
    //     exit(-1);
    // }


    // ///else 
    // {
    // //    printf("Udalo sie polaczyc!\n");
    //     //write(mySocket, )
    //     // while(1)
    //     // {
    //     //     printf("Wysylam\n");
    //     //     write(mySocket, "TUTAJ KLIENT", 50);
    //     //     sleep(1);
    //     // }
    //     //polaczenieJakoKlient(&mySocket, &port);
    //     losoweDane(&mySockaddrUN);

    //     for(int i = 0; i < iloscPolaczen; i++)
    //     {
    //         write(mySocket, &mySockaddrUN, sizeof(mySockaddrUN));
    //         sleep(1);
    //     }

    // }
    
    while(1)
    {
        int nfds = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);
        if(nfds == -1)
        {
            printf("Blad nfds, main - multiwriter\n");
            exit(-1);
        }

        printf("LICZBA NFDS: %d\n", nfds);

        for(int i = 0; i < nfds; i++)
        {
            if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP || !(events[i].events & EPOLLIN))
            {
                {
                    if ((close(events[i].data.fd)) == -1)
                    {
                        printf("Multiwriter - main - close error %d ", errno);
                        exit(-1);
                    }
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                    printf("\n\nevents flag\n");
                }
            }

            else
            {
                if (events[i].data.fd == server_fd)
                {
                    /*int incomfd = */
                    //acceptConnection(events[i].data.fd, epoll_fd);
                    akceptowaniePolaczenia(events[i].data.fd, &tablicaDeskryptorowLokal);

                    char buf[256];
                     read(server_fd, &buf, 30);
                    write(1, &buf, 30);
                    printf("polaczylo sie z LOCAL- acceptConnection\n");

                }
                else if (events[i].data.fd == socketClient)
                {
                    printf("CZYTANIE STURKTURY OLOLOLO\n");
                    czytanieStruktury(socketClient);
                }
            }
        }

    }  
    //sleep(100);

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

    nonBlock(tempFileDescriptor);
    dodanie_do_epoll(tempFileDescriptor, EPOLLIN | EPOLLET);

    **tablicaDeskryptorowLokal = tempFileDescriptor;
    printf("TABLICA DESKRYPTOROW: %d\n", **tablicaDeskryptorowLokal);
    (*tablicaDeskryptorowLokal) += 1;
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

      if ( (int)myStructUN.sun_family == -1)
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
      //probaPolaczenia(&myStructUN);
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