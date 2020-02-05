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

#define MAX 80
//#define PORT 8080

void czytanieParametrow(int argc, char** argv, int* sIloscPolaczen,
                         int* pPort, float* dOdstepCzasowy, float* tCalkowityCzasPracy);

int main(int argc, char** argv)
{
    int port;
    int iloscPolaczen;
    float odstepCzasowy;
    float calkowityCzasPracy;

    czytanieParametrow(argc, argv, &iloscPolaczen, &port,
         &odstepCzasowy, &calkowityCzasPracy);

    // utworzenie socketu

    int mySocket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in cli_addr; // struktura do socketa- klient

    cli_addr.sin_port = htons(port);
    cli_addr.sin_family = AF_INET;
    //cli_addr.sin_addr.s_addr = INADDR_ANY;
    cli_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(cli_addr.sin_zero, 0, 8);

    if ( connect(mySocket, (struct sockaddr*)&cli_addr, sizeof(cli_addr)) != 0)
    {
        printf("Nie udalo sie polaczyc\n");
        exit(-1);
    }

    else 
    {
        printf("Udalo sie polaczyc!\n");
        //write(mySocket, )
        while(1)
        {
            printf("Wysylam\n");
            write(mySocket, "TUTAJ KLIENT", 50);
            sleep(1);
        }
    }
    
    sleep(100);

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