//  **************************************************
//  *  pcdClient.c - inet pcd client implementation  *
//  **************************************************

#include <stdio.h>

#include <stdlib.h>

#include <sys/types.h>

#include <sys/socket.h>

#include <netdb.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include <string.h>

#include <unistd.h>

#define MAX_MSG 512

# define MAXCHAR 128

# define PORT 18081

int main(int argc, char * argv[]) {

  printf("\nINET Client Program\n");

  int clientFd;
  int rc;
  struct sockaddr_in serverAddr;
 

  int result;
  char buf[1024];
  int numBytes;
  char choice[128];
  char mesaj[128];
  char buffer[512];

  // create socket for client.
  clientFd = socket(AF_INET, SOCK_STREAM, 0);

  if (clientFd < 0) {
    printf("\nSocket creation failed!\n");
    exit(1);
  }


  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  serverAddr.sin_port = htons(PORT);


  result = connect(clientFd, (struct sockaddr * ) & serverAddr, sizeof(serverAddr));

  if (result < 0) {
    printf("\nFailed to establish connection with server!\n");
    exit(0);
  } else
    printf("\nConnected to server!\n");

  while (1) {

    printf("Aplicatie prognoza meteo\n");
    printf("1 - Afiseaza locatiile disponibile\n"); //afisare cand ma conectez
    printf("2 - Afiseaza prognoza meteo pentru o locatie\n"); //selecteaza film + rezervare
    printf("3 - Descarca raport detaliat\n");
    printf("0 - Terminare program.\n");
    printf("Your choice: ");
    scanf("%c", choice);
    // if ((alegere[0] < '0' || alegere[0] > '3'))
    // {
    //     printf("\n Alegere incorecta. Incearca din nou\n\n");
    // }

    // proceseaza o alegere corecta. in cazurile 1-3 se creeaza
    // un mesaj care e trimis serverului de administrare

    switch (choice[0]) {
    case '1':
      strcpy(mesaj, "1 ");
      break;
    
    case '2':
      strcpy(mesaj, "2");
      printf("\nChoose one location from the list: ");
      scanf("%s", choice);
      printf("\n-------------------------------------------\n");
      strcat(mesaj, choice);
      break;
    
    case '3':
      strcpy(mesaj, "3");
      printf("\nChoose one location from the list: ");
      scanf("%s", choice);
      printf("\n-------------------------------------------\n");
      strcat(mesaj, choice);
      break;
    
    case '0':
      printf("\nProgram will exit...\n");
      exit(0);
      break;
    
    default:
      break;
    }

    numBytes = send(clientFd, mesaj, strlen(mesaj), 0);
    if (numBytes <= 0) {
      perror("send() failed");
    }

    rc = recv(clientFd, buffer, MAX_MSG, 0);
    if (rc < 0) {
      perror("recv() failed..");
    } else if (rc == 0) {
      printf("Deconectat de la serverul UNIX.. \n");
    } else {

      buffer[rc] = 0;
      printf("\nServer response: %s\n\n", buffer);
    }

    printf("\nPress any key to return to menu\n");
    getchar();
    getchar();
  }

}