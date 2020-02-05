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
//#define PORT 8080

int server_fd; // deskryptor do servera
int epoll_fd; // deskryptor do epoll
int socketDoPolaczeniaDoKlienta;

int newsockfd; // deskryptor socketa od klienta

struct sockaddr_in address;
struct epoll_event ev; // struktura do epolla

struct epoll_event evTemp; // struktura do funkcji: dodanie_do_epoll

void tworzenie_serwer(int port);

void tworzenie_epoll();
void dodanie_do_epoll(int fileDesc, int typDoEpoll);

void czytanieParametrow(int argc, char** argv, int* port, char** prefiksSciezki);

void czytanieStruktury(int fileDescriptor);

void nonBlock(int fileDescriptor);
void nasluchiwanieServer(int fileDescriptor);
void akceptowaniePolaczenia(int fileDescriptor);
void probaPolaczenia(struct sockaddr_un* sockaddrStruct);
void polaczenieJakoKlient(int* socketTemp, int* port);

struct epoll_event events[MAX_EPOLL_EVENTS];

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
    dodanie_do_epoll(server_fd, EPOLLIN | EPOLLET);
    nasluchiwanieServer(server_fd);


    while(1)
    {
        int nfds = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);

        for(int i = 0; i < nfds; i++)
        {

            if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP || !(events[i].events & EPOLLIN))
            {
                printf("Blad nfds main\n");
               // exit(-1);
            }

            else if( events[i].data.fd == server_fd)
            {
                akceptowaniePolaczenia(events[i].data.fd);
            }

            else 
            {
                czytanieStruktury(events[i].data.fd);
            }
        }

        sleep(1);
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
    address.sin_port = htons( port ); 
    // dopisac memset address.sin_zero ( jest w kliencie )

    if ( bind(server_fd, (struct sockaddr *)&address, sizeof(address) ) < 0 )
    {
        printf("Blad bind - tworzenie serwera\n");
        exit(-3);
    }

    nonBlock(server_fd);
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

      probaPolaczenia(&myStructUN);

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
    if ( listen(server_fd, 5) < 0)
    {
        printf("Blad listen main \n");
        exit(-1);
    }
}

void akceptowaniePolaczenia(int fileDescriptor)
{
    struct sockaddr address;

    int addrlen = sizeof(address);
    int newsockfd;

    if (( newsockfd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) == -1)
    {
            printf("Blad accept\n");
            exit(-1);
    }

    nonBlock(newsockfd);
    dodanie_do_epoll(newsockfd, EPOLLIN | EPOLLET);
}

void probaPolaczenia(struct sockaddr_un* sockaddrStruct)
{
    //socketDoPolaczeniaDoKlienta;
    if ( (socketDoPolaczeniaDoKlienta = socket(AF_LOCAL, SOCK_STREAM, 0)) == -1 )
    {
        printf("Blad utworzenia socketa- probaPolaczenia\n");
        exit(-1);
    }

    if ( (connect(socketDoPolaczeniaDoKlienta,
         (struct sockaddr*)sockaddrStruct, sizeof(struct sockaddr_in))) == -1)
         {

        //printf("Nie udalo sie polaczyc z adresem: %s\n", sockaddrStruct->sun_path);
        // odeslac te strukture do klienta z parametrem sun_family = -1;
            printf("Nie udalo sie polaczyc z podanym adresem: %s\n",
                 sockaddrStruct->sun_path);

            // wyslac spowrotem do nadawcy tej struktury z 
            // flaga sun_family = -1
         }


    // udalo sie polaczyc, zwrocic te strukture do klienta- taka sama
    else
        {
            printf("Udalo sie polaczyc z adresem: %s\n", sockaddrStruct->sun_path);
        }
}


void polaczenieJakoKlient(int* socketTemp, int* port)
{
    *socketTemp = socket(AF_INET, SOCK_STREAM, 0);

    // sprawdzenie czy socket dziala

    // tworze strukture 
    struct sockaddr_in tempSockaddr;

    tempSockaddr.sin_family = AF_INET; 
    tempSockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    tempSockaddr.sin_port = htons( *port ); 


    connect(*socketTemp, (struct sockaddr*)&tempSockaddr, sizeof(tempSockaddr));
    // sprawdzic czy dziala
}