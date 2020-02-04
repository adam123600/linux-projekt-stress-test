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


#define PORT 8080

int server_fd; // deskryptor do servera
int epoll_fd; // deskryptor do epoll

int newsockfd; // deskryptor socketa od klienta

struct sockaddr_in address;
struct  epoll_event ev; // struktura do epolla

struct epoll_event evTemp; // struktura do funkcji: dodanie_do_epoll

void tworzenie_serwer(int port);

void tworzenie_epoll();
void dodanie_do_epoll(int fileDesc, int typDoEpoll);

void czytanieParametrow(int argc, char** argv, int* port, char** prefiksSciezki);

void struktura_Sockddr(int fileDescriptor);


int main (int argc, char** argv)
{

    int sockfd;
    int newsockfd;
    int portno;
    socklen_t clilen;
    char buffer[256];
    int n;

    char* prefiksSciezki = NULL;

    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;


    struct epoll_event event;

    int port;
    czytanieParametrow(argc, argv, &port, &prefiksSciezki);

    if ( port < 6000 || port > 65535)
    {
        printf("Podany PORT jest nieprawidlowy!\n");
        exit(-1);
    }


    tworzenie_serwer(port);


    if ( listen(server_fd, 5) < 0)
    {
        printf("Blad listen main \n");
        exit(-1);
    }

    int addrlen = sizeof(address);

    if (( newsockfd = accept(server_fd, (struct sockaddr *)&address,
        (socklen_t*)&addrlen)) < 0)
        {
            printf("Blad accept\n");
            exit(-1);
        }

    struct epoll_event *events;
    events = (struct epoll_event*)calloc(MAX_EPOLL_EVENTS,
                                 sizeof(struct epoll_event));
    if ( !events )
    {
        printf("Blad calloc main\n");
        exit(-1);
    }


    while(1)
    {
        int nfds = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);

        // if (nfds  == -1)
        // {
        //     printf("Blad nfds main\n");
        //     exit(-1);
        // }

        for (int i = 0; i < nfds; i++)
        {
            write(1, "ASD", strlen("ASD"));
        }
        printf("A\n");
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
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( port ); 

    if ( bind(server_fd, (struct sockaddr *)&address, sizeof(address) ) < 0 )
    {
        printf("Blad bind - tworzenie serwera\n");
        exit(-3);
    }
}

void struktura_Sockddr(int fileDescriptor)
{
      struct sockaddr_un structUn;

      read(fileDescriptor, &structUn, sizeof(structUn));

      // sprobowac wyslac
        // jesli tak to wyslac
        // else
            // poslac do autora
            // z flaga w sun_family -1
}


void dodanie_do_epoll(int fileDesc, int typDoEpoll)
{

    evTemp.data.fd = fileDesc;
    evTemp.events = typDoEpoll;
    
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, evTemp.data.fd, &evTemp);
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
}