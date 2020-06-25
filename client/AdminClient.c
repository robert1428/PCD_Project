#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SERVER_FILE	"/tmp/unixFile"

#define MAX_MSG		512
#define MIN_MSG		8

int main(int argc, char *argv[])
{

    int clientFd, err, numBytes, rc, i=1;
    char buffer[512];
    struct sockaddr_un serverAddr;
    char alegere[128], numeFilm[128];
    char fd[8], c;
    char mesaj[128];

    clientFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (clientFd < 0)
    {
        perror("Eroare socket()..");
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sun_family = AF_UNIX;
    strcpy(serverAddr.sun_path, SERVER_FILE);

    err = connect(clientFd, (struct sockaddr *)&serverAddr, SUN_LEN(&serverAddr)); // conectare la server
    if (err < 0)
    {
        perror("\n Eroare de conectare la server UNIX..");
    }
    else
    {
        printf("\n Clientul de administrare server UNIX conectat..\n");
    }

    // afisare menu principal
    while(1)
    {

        printf("  Menu administrare server UNIX: \n\n");
        printf("  1 - Afisare fd-uri clienti INET. \n");
        printf("  2 - Afisare IP si port client. \n");
        printf("  3 - Deconectare client. \n");
        printf("  4 - Adauga film nou. \n");
        printf("  0 - Terminare program.  \n");

        printf( " Alege = ");
        scanf("%c", alegere);

        if ((alegere[0] < '0' || alegere[0] > '5'))
        {
            printf("\n Alegere incorecta. Incearca din nou\n\n");
            continue;
        }

        // proceseaza o alegere corecta. in cazurile 1-3 se creeaza
        // un mesaj care e trimis serverului de administrare

        switch(alegere[0])
        {
        case '1':
            strcpy(mesaj, "1 000");
            break;
        case '2':
            printf("\n Specifica fd client = ");
            scanf("%s", fd);
            strcpy(mesaj, "2 ");
            strncat(mesaj, fd, 3);
            break;
        case '3':
            printf("\n Specifica fd client = ");
            scanf("%s", fd);
            strcpy(mesaj, "3 ");
            strncat(mesaj, fd, 3);
            break;
        case '4':
            strcpy(mesaj, "4");
            printf("\nAdauga un film nou: ");
            printf("\n Specifica nume film: ");
            scanf("%s", numeFilm);
            do
            {
                switch(i)
                {
                case 1:
                    printf("\n Adauga prima ora de difuzare (hh:mm): ");
                    break;
                case 2:
                    printf("\n Adauga a doua ora de difuzare (hh:mm): ");
                    break;
                case 3:
                    printf("\n Adauga a treia ora de difuzare (hh:mm): ");
                    break;

                }
                scanf("%s", alegere);

                if(alegere[2] == ':')
                {

                    if(strlen(alegere)!=5)
                    {
                        strcpy(mesaj, "");
                        printf("\n Nu s-a putut adauga filmul! Incearca din nou..");
                        break;
                    }
                    else
                    {
                        printf("\nAi ales ora %s", alegere);
                        strcat(mesaj, alegere);
                        i++;
                    }
                }
            }
            while(i!=4);

            strcat(mesaj,numeFilm);
            printf("\nMesajul este: %s", mesaj);
            break;
        case '0':
            printf("\n Terminare program\n");
            exit(0);
            break;
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
            printf("Deconectat de la serverul UNIX.. \n");
        }
        else
        {
            // display the message received from the admin server
            buffer[rc] = 0;
            printf("\n Mesaj de la server UNIX: %s\n", buffer);
        }

        printf("\n Apasa o tasta pentru a continua \n");
        getchar();
        getchar();
    }
}
