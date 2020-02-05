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

//#define MAX 80
int server_fd;

struct sockaddr_in address;




void czytanieParametrow(int argc, char** argv, int* sIloscPolaczen,
            int* pPort, float* dOdstepCzasowy, float* tCalkowityCzasPracy);

void tworzenie_serwer(int port);
void nonBlock(int fileDescriptor);

void losoweDane(struct sockaddr_un* sockaddrStruct);

int main(int argc, char** argv)
{
    int port;
    int iloscPolaczen;
    float odstepCzasowy;
    float calkowityCzasPracy;

//  struct sockaddr_un* mySockaddr;
   //mySockaddr.sun_family = AF_LOCAL;
   //mySockaddr.sun_path =  // tutaj random "\0, 107 bajtow";

    struct sockaddr_un mySockaddrUN;
    losoweDane(&mySockaddrUN);


    czytanieParametrow(argc, argv, &iloscPolaczen, &port,
         &odstepCzasowy, &calkowityCzasPracy);

    write(1, mySockaddrUN.sun_path, 108);
    

    // utworzenie socketu

    int mySocket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in cli_addr; // struktura do socketa- klient

    cli_addr.sin_port = htons(port);
    cli_addr.sin_family = AF_INET;
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