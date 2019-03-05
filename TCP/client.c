#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>


#define PACKET_SIZE 1000

extern int errno;

/* portul de conectare la server*/
int port;
char  * data;
int dataSize;
int sentPackages = 0;

void readFile()
{
  FILE * fileptr;
  long filelen;
  fileptr = fopen("output.txt","rb");
  size_t retVal;

  fseek(fileptr, 0, SEEK_END);       // go to the end of the file
  filelen = ftell(fileptr);          // get file length     
  rewind(fileptr);                   // go to the start of the file

  data = (char *) calloc((filelen+1)*sizeof(char),1); // allocate memory for the content of the file
  retVal = fread(data,sizeof(char),filelen, fileptr); // read file
  dataSize = retVal;
  fclose(fileptr); // Close the file

  printf("%d",(int)retVal);
  fflush(stdout);
}

int sendPacketWrapper(char * info,int socketDescriptor,int dimension)
{ 
  int retVal = -1;
  retVal = write(socketDescriptor,info,dimension);

  if(retVal < 0)
    return -1;

  return 0;
}

int receivePacketWrapper(int socketDescriptor,int dimension)
{ 
  int retVal = -1;
  char * info = (char *) calloc(5,1);

  retVal = read(socketDescriptor,info,dimension);

  if(retVal < 0)
    return -1;
  else
  {
    if(strcmp(info,"ACK") == 0)
      return 0;
    else return 1;
  }
}

void doStopAndWait(int socketDescriptor)
{ 
  char * msg = (char *) calloc(20,1);
  strcpy(msg,"STAWT");

  if (write (socketDescriptor, msg, strlen(msg)) <= 0)
  {
      printf("Eroare la trimiterea optiunii \n");
      return;
  }
  else
  {
    char * currentPacket = (char *) calloc(PACKET_SIZE+1,1);
    printf("**************** %d ************ \n",dataSize);
    int counter = 1;
    int done=0;
    while(counter * PACKET_SIZE < dataSize)
    { 
        sentPackages += 1;
        memcpy(currentPacket,data,PACKET_SIZE);
        if(sendPacketWrapper(currentPacket,socketDescriptor,PACKET_SIZE) == -1)
        {
          printf("Eroare la trimiterea unui pachet \n");
          return;
        }
        printf("Am trimis pachetul %d \n",counter);
        data += PACKET_SIZE;
        counter ++;

        /* astept ACK*/
        if(receivePacketWrapper(socketDescriptor,3) == 0) //daca am primit ack continui
          continue;
    }

    data -= PACKET_SIZE;
  
    memcpy(currentPacket,data,dataSize-(counter-1)*PACKET_SIZE);
    if(sendPacketWrapper(currentPacket,socketDescriptor,PACKET_SIZE) == -1)
        {
          printf("Eroare la trimiterea unui pachet \n");
          return;
        }
    sentPackages += 1;    
  }

  printf("done \n");
  fflush(stdout);
}

void doStreaming(int socketDescriptor)
{
  char * msg = (char *) calloc(20,1);
  strcpy(msg,"STRMG");

  printf("test \n");
  if (write (socketDescriptor, msg, strlen(msg)) < 0)
  {
      printf("Eroare la trimiterea optiunii \n");
      return;
  }
  else
  { 
    char * currentPacket = (char *) calloc(PACKET_SIZE+1,1);
    printf("**************** %d ************ \n",dataSize);
    int counter = 1;
    int done=0;
    while(counter * PACKET_SIZE < dataSize)
    {
        sentPackages += 1;
        memcpy(currentPacket,data,PACKET_SIZE);
        if(sendPacketWrapper(currentPacket,socketDescriptor,PACKET_SIZE) == -1)
        {
          printf("Eroare la trimiterea unui pachet \n");
          return;
        }
        data += PACKET_SIZE;
        counter ++;
    }
    data -= PACKET_SIZE;

    memcpy(currentPacket,data,dataSize-(counter-1)*PACKET_SIZE);
    if(sendPacketWrapper(currentPacket,socketDescriptor,PACKET_SIZE) == -1)
        {
          printf("Eroare la trimiterea unui pachet \n");
          return;
        }

    sentPackages += 1;
    printf("done");
    fflush(stdout);
  }
}

int main (int argc, char *argv[])
{
  int sd;			                // descriptorul de socket
  struct sockaddr_in server;	// structura folosita pentru conectare 
  char * msg= (char *) calloc(5,1);

  /* exista toate argumentele in linia de comanda? */
  if (argc != 3)
    {
      printf("Nu ati introdus toate datele input \n");
      return -1;
    }

  port = atoi (argv[2]);

  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("Eroare la socket().\n");
      return errno;
    }

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr(argv[1]);
  server.sin_port = htons (port);
  
  if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
      perror ("[client]Eroare la connect().\n");
      return errno;
    }

  readFile();

  printf ("[client]Introduceti mecanismul dorit pentru clientul TCP: ");
  fflush (stdout);

  read (0, msg, 5);
  printf("am citit de la key %s \n ",msg);

  if (strcmp(msg,"STRMG") == 0)
      {
        doStreaming(sd);
        printf("Am trimis %d pachete \n",sentPackages);
      }
  else if (strcmp(msg,"STAWT") == 0)
          {
            doStopAndWait(sd);
            printf("Am trimis %d pachete \n",sentPackages);
          }
}

