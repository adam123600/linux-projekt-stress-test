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





#define PORT 8081

int server_fd; // deskryptor do servera
struct sockaddr_in address;

void tworzenie_serwera(int port);
void struktura_Sockddr(int fileDescriptor);


int main (int argc, char** argv)
{
    // FUNKCJE Z KTORYCH TTRZEBA SKORZYSTAC:
    // getopt

    int sockfd;
    int newsockfd;
    int portno;
    socklen_t clilen;
    char buffer[256];
    int n;

    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;

    struct epoll_event event;

    tworzenie_serwera(5);


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


    sleep(5);
    write(newsockfd, "ASD", strlen("ASD"));
    sleep(5);

    close(newsockfd);
    close(sockfd);

    return 0;

}

void tworzenie_serwera(int port)
{
   
    if ( (server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0 )
    {
        printf("Blad - tworzenie serwera\n");
        exit(-2);
    }

    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( PORT ); 

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