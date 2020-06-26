#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <sys/types.h>

#include <sys/socket.h>

#include <sys/un.h>

#include <unistd.h>

#define SERVER_FILE "/tmp/unixFile"

#define MAX_MSG 512
#define MIN_MSG 8

int main(int argc, char *argv[])
{

  int clientFd, err, numBytes, rc, i = 1;
  char buffer[512];
  struct sockaddr_un serverAddr;
  char choice[128];
  char locationName[128];
  char longitude[15];
  char latitude[15];
  char fd[8], c;
  char mesaj[128];

  clientFd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (clientFd < 0)
  {
    perror("Erorr creating socket()..");
  }

  memset(&serverAddr, 0, sizeof(serverAddr));
  serverAddr.sun_family = AF_UNIX;
  strcpy(serverAddr.sun_path, SERVER_FILE);

  err = connect(clientFd, (struct sockaddr *)&serverAddr, SUN_LEN(&serverAddr)); // conectare la server
  if (err < 0)
  {
    printf("\n Error connecting to UNIX server\n");
    exit(1);
  }
  else
  {
    printf("\n Admin client connected \n");
  }

  while (1)
  {

    printf("  Menu administrare server UNIX: \n\n");
    printf("  1 - Afisare fd-uri clienti INET. \n");
    printf("  2 - Add mew location. \n");
    printf("  0 - End application.  \n");

    printf("Your choice = ");
    scanf("%s", choice);

    if ((choice[0] < '0' || choice[0] > '5'))
    {
      printf("\nPlease choose a valid option \n\n");
      continue;
    }

    switch (choice[0])
    {
    case '1':
      strcpy(mesaj, "1 000");
      break;
    case '2':
      strcpy(mesaj, "2");
      printf("\n Enter location name ");
      scanf("%s", locationName);
      printf("\n Enter location longitude ");
      scanf("%s", longitude);
      printf("\n Enter location latitude ");
      scanf("%s", latitude);

      strcat(mesaj, " ");
      strcat(mesaj, locationName);
      strcat(mesaj, " ");
      strcat(mesaj, longitude);
      strcat(mesaj, " ");
      strcat(mesaj, latitude);
      strcat(mesaj, "\n");

      break;

    case '0':
      exit(0);
    default:
      break;
    }

    numBytes = send(clientFd, mesaj, strlen(mesaj), 0);
    if (numBytes <= 0)
    {
      perror("send() failed");
    }

    rc = recv(clientFd, buffer, MAX_MSG, 0);
    if (rc < 0)
    {
      perror("recv() failed..");
    }
    else if (rc == 0)
    {
      printf("Deconectat de la Server .. \n");
    }
    else
    {

      buffer[rc] = 0;
      printf("\n Mesaj de la server : %s\n", buffer);
    }

    printf("\n Apasa o tasta pentru a continua \n");
    getchar();
    getchar();
  }
}