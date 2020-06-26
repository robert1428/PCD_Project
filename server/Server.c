#include <sys/un.h>

#include <sys/socket.h>

#include <stddef.h>

#include <errno.h>

#include <pthread.h>

#include <sys/un.h>

#include <stdlib.h>

#include <string.h>

#include <unistd.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include <sys/types.h>

#include <sys/socket.h>

#include <sys/select.h>

#include <json-c/json.h>

#include <pthread.h>

#include <curl/curl.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include <json-c/json.h>

#include <stdio.h>

#include <fcntl.h>

#define UNIX_FILE "/tmp/unixFile"

#define INETPORT 18081
#define SOAPPORT 18082
#define UNIXSOCKET "/tmp/unixds"

#define MAXBLOCKED 128

#define NUM_MAX_CLIENTS 256

#define MAXCHAR 256

#define DARK_SKY_API_URL "https://api.darksky.net/forecast/32f1f87b5680f82c0b095474b9ce7e66/"
typedef struct
{
  int fd;
  char IP[16];
  __uint16_t port;
} inetClient;

inetClient clientList[512];
int maxClient;

struct Location
{
  char name[40];
  double lon;
  double lat;
};

struct Forecast
{
  char summary[100];
  double temperature;
  double pressure;
  int uvIndex;
  double windSpeed;
};

FILE *fileBlocked;
char blockedIps[NUM_MAX_CLIENTS][16]; //lista de clienti blocati
int numBlocked = 0;

int processUnixClientRequest(int bytes, char client[512], char mesaj[512]);
int processClientRequest(int cliFd, char mesaj[512], char *msg);
char *createStringFromLocationsFIle();
struct Location createLocation(char *line);
char *getNthLineFromFile(int lineNumber);
void sendAPIRequest(char *coordinates);
struct Forecast createForecast();
char *createForecastString(struct Forecast forecast);
void *unix_main(void *args)
{

  printf("[+]Server is running\n");

  int serverFd = -1, clientFd = -2;
  int error, nBytes, numSent;
  char cmdClient[512];
  char mesaj[512];
  struct sockaddr_un serverAddr;

  serverFd = socket(AF_UNIX, SOCK_STREAM, 0);

  if (serverFd < 0)
  {
    perror("UNIX socket() error..");
  }

  memset(&serverAddr, 0, sizeof(serverAddr));
  serverAddr.sun_family = AF_UNIX;
  strcpy(serverAddr.sun_path, UNIX_FILE);

  int opt = 1;
  if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
  {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }
  if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEPORT, (char *)&opt, sizeof(opt)) < 0)
  {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  error = bind(serverFd, (struct sockaddr *)&serverAddr, SUN_LEN(&serverAddr));

  if (error < 0)
  {
    perror("UNIX bind() error..");
  }

  error = listen(serverFd, 10);

  if (error < 0)
  {
    perror("UNIX listen() error..");
  }

  int tries = 10;
  while (tries)
  {

    if (clientFd < 0)
    {
      if ((clientFd = accept(serverFd, NULL, NULL)) == -1)
      {
        perror("Server accept() error..\n");
        break;
      }
    }
    else
    {
      while ((nBytes = read(clientFd, cmdClient, sizeof(cmdClient))) > 0)
      {
        cmdClient[nBytes] = 0;
        printf("Server = read %d bytes: %s\n", nBytes, cmdClient);

        int len = processUnixClientRequest(nBytes, cmdClient, mesaj);

        numSent = send(clientFd, mesaj, len, 0);
        if (numSent < 0)
        {
          perror("Server send() error..");
          break;
        }
      }
      if (nBytes < 0)
      {
        perror("Client read error, the connection will be closed.\n");
        close(clientFd);
        clientFd = -2;
        continue;
      }
      else if (nBytes == 0)
      {
        printf(" ... The client closed the connection.\n");
        close(clientFd);
        clientFd = -2;
        continue;
      }
    }
    --tries;
  }

  if (serverFd > 0)
    close(serverFd);
  if (clientFd > 0)
    close(clientFd);

  unlink(UNIX_FILE);
}

int processUnixClientRequest(int numChars, char *buffer, char *msg)
{
  int cmdType = buffer[0];
  int numConnected = 0;
  char temp[256];
  printf("\n UNIX = comanda primita = %s, type: %c.\n", buffer, cmdType);
  bzero(msg, strlen(msg));
  int i, theFd, found;

  switch (cmdType)
  {
  case '1':
    strcpy(msg, "1");
    for (i = 0; i < maxClient; i++)
    {
      if (clientList[i].fd > 0)
      {
        strcat(msg, ":");
        snprintf(temp, 3, "%d", clientList[i].fd);
        strcat(msg, temp);
        numConnected++;
      }
    }
    if (numConnected == 0)
      strcat(msg, " - No clients connected");
    break;
  case '2':
    printf("%s\n", buffer);

    int fd = open("locations.txt", O_WRONLY | O_APPEND | O_CREAT, 0644);

    if (fd == -1)
    {
      printf("Fisierul nu a putut fi deschis/creat!");
      exit(1);
    }

    write(fd, buffer, strlen(buffer));
    strcat(msg, "Added Location");

    close(fd);

    break;

  default:

    printf("Command is invalid, it will be ignored\n");
    break;
  }
  printf("\nMessage for UNIX client: %s", msg);
  return strlen(msg);
}

void *inet_main(void *args)
{

  int serverFd, clientFd, error, i, maxFd, numReady, numBytes, numSent;
  fd_set readSet;

  struct sockaddr_in
      serverAddr,
      clientAddr;

  socklen_t addr_size;

  char mesaj[512];
  char clientMsg[512];
  pid_t childpid;

  serverFd = socket(AF_INET, SOCK_STREAM, 0);
  if (serverFd < 0)
  {
    printf("[-]Error in connection..\n");
    exit(1);
  }
  printf("[+]Server Socket is created.\n");

  memset(&serverAddr, '\0', sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(INETPORT);
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

  error = bind(serverFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
  if (error < 0)
  {
    printf("[-]Error in binding.\n");
    exit(1);
  }
  else
  {
    printf("[+]Bind to port %d\n", INETPORT);
  }

  if (listen(serverFd, 10) == 0)
  {
    printf("[+]Listening....\n");
  }
  else
  {
    printf("[-]Error in binding.\n");
  }

  // for (i = 0; i < maxClient; i++) {
  //   clientList[i].fd = 0;
  // }

  // char * blockedSpec = "../theServer/blockedIPs.txt";
  // fileBlocked = fopen(blockedSpec, "a+");
  // if (!fileBlocked) {
  //   printf("INET * could not open the blocked ip's file\n");
  // } else {
  //   printf("INET - blocked ip's:\n\n");
  //   while (fgets(blockedIps[numBlocked], MAXBLOCKED, fileBlocked)) {
  //     printf("%s", blockedIps[numBlocked]);
  //     numBlocked++;
  //   }
  // }

  FD_ZERO(&readSet);
  FD_SET(serverFd, &readSet);
  maxFd = serverFd + 1;
  addr_size = sizeof(clientAddr);

  while (1)
  {

    printf("INET - ready for ordinary clients to connect().\n");
    FD_SET(serverFd, &readSet);
    numReady = select(maxFd + 1, &readSet, NULL, NULL, NULL);

    if (FD_ISSET(serverFd, &readSet))
    {
      clientFd = accept(serverFd, (struct sockaddr *)&clientAddr, &addr_size);
      char *clientIP = inet_ntoa(clientAddr.sin_addr);
      printf("INET - accepted connection. fd = %d, IP = %s, will check if it is blocked.\n", clientFd, clientIP);

      //int isBlocked = 0;

      //for (i = 0; i < NUM_MAX_CLIENTS; i++) {
      //if (!strcmp(clientIP, blockedIps[i])) {
      // isBlocked = 1;
      // close(clientFd);
      // printf("INET - this IP: %s is blocked. connection request denied.\n", clientIP);
      // break;
      //}
      // }

      //if (!isBlocked)
      {
        // printf("INET - the IP: %s is not blocked.\n", clientIP);

        for (i = 0; i < NUM_MAX_CLIENTS; i++)
        {
          if (clientList[i].fd == 0)
          {
            clientList[i].fd = clientFd;
            strcpy(clientList[i].IP, clientIP);
            clientList[i].port = clientAddr.sin_port;
            if (i >= maxClient)
              maxClient = i + 1;
            break;
          }
        }

        if (i == NUM_MAX_CLIENTS)
        {
          printf("INET - * too many clients, will exit..\n");
          exit(1);
        }

        FD_SET(clientFd, &readSet);

        if (clientFd > maxFd)
          maxFd = clientFd;
      }
      if (--numReady <= 0)
        continue;
    }

    for (i = 0; i < NUM_MAX_CLIENTS; i++)
    {
      if (clientList[i].fd <= 0)
        continue;

      if (FD_ISSET(clientList[i].fd, &readSet))
      {
        if ((numBytes = recv(clientList[i].fd, clientMsg, MAXCHAR, 0)) <= 0)
        {
          FD_CLR(clientList[i].fd, &readSet);
          close(clientList[i].fd);
          printf("INET ** conection with client at IP: %s, fd = %d - closed.\n", clientList[i].IP, clientList[i].fd);
          clientList[i].fd = 0;
        }
        else
        {
          printf("INET - received message: %s from client with IP: %s, fd = %d.\n", clientMsg, clientList[i].IP, clientList[i].fd);

          int len = processClientRequest(clientList[i].fd, clientMsg, mesaj);
          numSent = send(clientList[i].fd, mesaj, len, 0);
          if (numSent < 0)
          {
            perror("UNIX send() error..");
            break;
          }
        }
      }
    }
  }
}

char *createForecastString(struct Forecast forecast)
{
  char result[1024];
  char wind[10];
  char temperature[10];
  char pressure[10];
  strcpy(result, "\n");
  strcat(result, forecast.summary);
  strcat(result, "\nTemperatura: ");
  snprintf(wind, 10, "%.2lf km/h", forecast.windSpeed);
  snprintf(temperature, 10, "%.2lf C", ((forecast.temperature - 32) * 5 / 9));
  snprintf(pressure, 10, "%.2lf", forecast.pressure);

  strcat(result, temperature);
  strcat(result, "\nPresiune: ");
  strcat(result, pressure);
  strcat(result, "\nViteza vantului: ");
  strcat(result, wind);
}

void sentFile(int sockfd)
{
  char buff[1024];

  FILE *fp;
  fp = fopen("Raport.json", "r");
  if (fp == NULL)
  {
    printf("Error IN Opening File .. \n");
    return;
  }

  while (fgets(buff, 1024, fp) != NULL)
    write(sockfd, buff, sizeof(buff));

  fclose(fp);

  printf("File Sent successfully !!! \n");
}

struct Forecast createForecast()
{

  FILE *fp;
  struct Forecast forecast;

  char buffer[30000];
  struct json_object *parsed_json;
  struct json_object *currently;
  struct json_object *summary;
  struct json_object *temperature;
  struct json_object *uvIndex;
  struct json_object *wind;
  struct json_object *pressure;

  fp = fopen("Raport.json", "r");
  fread(buffer, 30000, 1, fp);
  fclose(fp);

  parsed_json = json_tokener_parse(buffer);

  json_object_object_get_ex(parsed_json, "currently", &currently);

  json_object_object_get_ex(currently, "summary", &summary);
  json_object_object_get_ex(currently, "temperature", &temperature);
  json_object_object_get_ex(currently, "uvIndex", &uvIndex);
  json_object_object_get_ex(currently, "pressure", &pressure);
  json_object_object_get_ex(currently, "wind", &wind);

  strcpy(forecast.summary, json_object_get_string(summary));
  forecast.temperature = json_object_get_double(temperature);
  forecast.uvIndex = json_object_get_int(uvIndex);
  forecast.pressure = json_object_get_double(pressure);
  forecast.windSpeed = json_object_get_double(wind);
  //printf("Age: %d\n", json_object_get_int(age));
  //printf("time: %d\n", json_object_get_int(time));
  return forecast;
}

void sendAPIRequest(char *coordinates)
{
  CURL *curl;
  FILE *fp;
  int result;

  char target[100] = DARK_SKY_API_URL;
  strcat(target, coordinates);

  printf("%s\n", target);

  fp = fopen("Raport.json", "wb");

  curl = curl_easy_init();

  curl_easy_setopt(curl, CURLOPT_URL, target);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

  result = curl_easy_perform(curl);

  if (result == CURLE_OK)
    printf("Download successfull!\n");
  else
    printf("ERROR: %s\n", curl_easy_strerror(result));

  fclose(fp);

  curl_easy_cleanup(curl);
}

int processClientRequest(int cliFd, char *buffer, char *msg)
{

  int cmdType = buffer[0];
  int numConnected = 0;
  char temp[512], user[512];
  int locationNumber;
  char latLonString[100];
  char lon[30];
  char lat[30];
  char *line;
  struct Location desiredLocation;
  struct Forecast forecast;

  printf("\n UNIX = comanda primita = %s, type: %c.\n", buffer, cmdType);
  bzero(msg, strlen(msg));
  strcat(msg, "\n");

  switch (cmdType)
  {
  case '1':
    strcat(msg, createStringFromLocationsFIle());

    break;
  case '2':
    locationNumber = buffer[1] - '0';
    printf("%d\n", locationNumber);
    line = getNthLineFromFile(locationNumber);
    desiredLocation = createLocation(line);
    snprintf(lat, 50, "%lf", desiredLocation.lat);
    snprintf(lon, 50, "%lf", desiredLocation.lon);
    strcpy(latLonString, lon);
    strcat(latLonString, ",");
    strcat(latLonString, lat);

    sendAPIRequest(latLonString);
    forecast = createForecast();
    strcpy(msg, "\n");
    strcat(msg, desiredLocation.name);
    strcat(msg, createForecastString(forecast));
    break;

  case '3':
    locationNumber = buffer[1] - '0';
    printf("%d\n", locationNumber);
    line = getNthLineFromFile(locationNumber);
    desiredLocation = createLocation(line);
    snprintf(lat, 50, "%lf", desiredLocation.lat);
    snprintf(lon, 50, "%lf", desiredLocation.lon);
    strcpy(latLonString, lon);
    strcat(latLonString, ",");
    strcat(latLonString, lat);

    sendAPIRequest(latLonString);

    sentFile(cliFd);

    break;

  default:
    printf("Command is invalid, it will be ignored..\n");
    break;
  }

  return strlen(msg);
}
char *getNthLineFromFile(int lineNumber)
{

  const char filename[] = "locations.txt";
  FILE *file = fopen(filename, "r");
  int count = 0;
  if (file != NULL)
  {
    char line[256];
    char *buffer;
    while (fgets(line, sizeof line, file) != NULL)
    {
      if (count == lineNumber - 1)
      {
        buffer = (char *)malloc(sizeof(char) * strlen(line));
        strcpy(buffer, line);
        return buffer;
      }
      else
      {
        count++;
      }
    }
    fclose(file);
  }
  else
  {
  }
}

struct Location createLocation(char *line)
{

  struct Location location;
  char buff[100];
  strcpy(buff, line);
  int init_size = strlen(buff);
  char delim[] = " ";

  char *ptr = strtok(buff, delim);
  ptr = strtok(NULL, delim);

  strcpy(location.name, ptr);

  double lon;
  ptr = strtok(NULL, delim);
  sscanf(ptr, "%lf", &lon);
  location.lon = lon;

  double lat;
  ptr = strtok(NULL, delim);
  sscanf(ptr, "%lf", &lat);
  location.lat = lat;

  return location;
}

char *createStringFromLocationsFIle()
{
  long length;
  FILE *f = fopen("locations.txt", "rb");
  char *buffer = 0;

  if (f)
  {
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = malloc(length);
    if (buffer)
    {
      fread(buffer, 1, length, f);
    }
    fclose(f);
  }

  return buffer;
}

int main()
{
  int inetPort;
  pthread_t
      unixThread, // UNIX Thread: componenta UNIX
      inetThread, // INET Thread: componenta INET
      soapThread; // SOAP Thread: componenta SOAP

  inetPort = INETPORT;

  unlink(UNIX_FILE);
  pthread_create(&unixThread, NULL, unix_main, UNIXSOCKET);

  inetPort = INETPORT; //portul utilizat pentru inet
  pthread_create(&inetThread, NULL, inet_main, &inetPort);

  //portul utilizat pentru soap
  //  pthread_create (&soapThread, NULL, soap_main, &soapPort);

  pthread_join(unixThread, NULL);
  pthread_join(inetThread, NULL);
  //  pthread_join(soapThread, NULL);
  unlink(UNIX_FILE);

  // char *str;
  // str = createStringFromLocationsFIle();
  // printf("%s", str);

  // char *loc;

  // loc = getNthLineFromFile(3);
  // struct Location location = createLocation(loc);
  // printf ("%s", location.name);

  // struct Forecast forecast = createForecast();
  // char* result;
  // result = createForecastString(forecast);
  // printf("%s\n", result);
  // sendAPIRequest("45,25");
  return 0;
}