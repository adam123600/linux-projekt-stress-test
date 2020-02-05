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


//    int addrlen = sizeof(address);

    // if (( newsockfd = accept(server_fd, (struct sockaddr *)&address,
    // (socklen_t*)&addrlen)) == -1)
    //     {
    //         printf("Blad accept\n");
    //         exit(-1);
    //     }

    // struct epoll_event *events;
    // events = (struct epoll_event*)calloc(MAX_EPOLL_EVENTS,
    //                              sizeof(struct epoll_event));
    // if ( !events )
    // {
    //     printf("Blad calloc main\n");
    //     exit(-1);
    // }
    printf("Przed whilem!\n");

    while(1)
    {
        //int nfds;
        int nfds = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);
        printf("Przed epoll wait\n");
        printf("po epoll wait");

        for(int i = 0; i < nfds; i++)
        {
            printf("Petla for\n");

            if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP || !(events[i].events & EPOLLIN))
            {
                printf("Blad nfds main\n");
                exit(-1);
            }

            else if( events[i].data.fd == server_fd)
            {
                printf("polczylo sie - ackeptowaniePolaczenia\n");
                akceptowaniePolaczenia(events[i].data.fd);
                read(events[i].data.fd, buffer, 50);
                printf("%s\n", buffer);
            }

            else 
            {
                // czytanie struktury 
                printf("czytanie struktury \n");
                czytanieStruktury(events[i].data.fd);
            }
        }

        
        
        //read()

        sleep(1);


        // if (nfds  == -1)
        // {
        //     printf("Blad nfds main\n");
        //     exit(-1);
        // }

        //for (int i = 0; i < nfds; i++)
       // {
        //    write(1, "ASD", strlen("ASD"));
        //}


        //printf("A\n");
       // sleep(1);

    }


    printf("Socket: %d\n", newsockfd);

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
    //   struct sockaddr_un structUn;
         printf("czytanie struktury \n");

    //   if ( read(fileDescriptor, &structUn, sizeof(structUn)) != sizeof(structUn))
    //   {
    //       printf("Blad czytania struktury - czytanieStruktury \n");
    //       exit(-1);
    //   }

    //  printf("czytanie struktury 2\n");
    //  printf("%s %d\n", structUn.sun_path, structUn.sun_family);

    char tempbuffer[500];

    read(fileDescriptor, tempbuffer, 500);

    printf("%s\n", tempbuffer);








      // sprobowac wyslac 
        // jesli tak to wyslac
        // else
            // poslac do autora
            // z flaga w sun_family -1
}


void dodanie_do_epoll(int fileDescriptor, int typDoEpoll)
{
    printf("dodawanie do epoll AAAA\n");

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