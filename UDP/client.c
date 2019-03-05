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

int port;
char  * data;
int dataSize;

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

 /*
  fileptr = fopen("replica.dat","wb");
  fwrite(data,filelen,1,fileptr); //write replica
  fclose(fileptr);
  */
}

int sendPacketWrapper(char * info,int socketDescriptor,int dimension, struct sockaddr_in server)
{ 
  int retVal = -1;
  int length = sizeof(server);

  retVal = sendto (socketDescriptor, info, dimension,0, (struct sockaddr*)&server, length);

  if(retVal <= 0)
  {
    printf("%d \n",errno);
    return -1;
  }
  return 0;
}

void doStreaming(int sd,struct sockaddr_in server)
{
  printf("Do Streaming");
  int counter = 0;
  char * msg = (char *) calloc(20,1);
  strcpy(msg,"STRMG");
  int length = sizeof(server);

  if (sendto (sd, msg, 5,0, (struct sockaddr*)&server, length) <= 0)
    {
      perror ("[client]Eroare la sendto() spre server.\n");
      return ;  
    }
  else
  { 
    char * currentPacket = (char *) calloc(PACKET_SIZE+1,1);
    printf("**************** %d ************ \n",dataSize);
    counter = 0;
    int done=0;
    while(counter * PACKET_SIZE < dataSize)
    {
        memcpy(currentPacket,data,PACKET_SIZE);
        if(sendPacketWrapper(currentPacket,sd,PACKET_SIZE,server) == -1)
        {
          printf("Eroare la trimiterea unui pachet \n");
          return;
        }
     //   printf("Am trimis pachetul %d \n",counter);
        data += PACKET_SIZE;
        counter ++;
    }
    data -= PACKET_SIZE;
  //  printf("am parasit bucla");
  //  fflush(stdout);

    memcpy(currentPacket,data,dataSize-(counter-1)*PACKET_SIZE);
    if(sendPacketWrapper(currentPacket,sd,PACKET_SIZE,server) == -1)
        {
          printf("Eroare la trimiterea unui pachet \n");
          return;
        }
    strcpy(currentPacket,"STOP");

    if(sendPacketWrapper(currentPacket,sd,4,server) == -1)
    {
      printf("Eroare la trimiterea pachetului de STOP \n");
      return;
    }
  }

  printf("Am trimis %d pachete",counter);
}

int receivePacketWrapper(int socketDescriptor,int dimension,struct sockaddr_in server)
{ 
  int retVal = -1;
  char * info = (char *) calloc(4,1);
  int length = sizeof(server);

  retVal=recvfrom (socketDescriptor, info, dimension,0,(struct sockaddr*)&server, &length);

  if(retVal <= 0)
    return -1;
  else
  {
    if(strcmp(info,"ACK") == 0)
      return 0;
    else return 1;
  }
}

void doStopAndWait(int sd,struct sockaddr_in server)
{
  char * msg = (char *) calloc(20,1);
  strcpy(msg,"STAWT");
  int length = sizeof(server);
  
  if (sendto (sd, msg, 5,0, (struct sockaddr*)&server, length) <= 0)
    {
      perror ("[client]Eroare la sendto() spre server.\n");
      return ;
  }
  else
  {
    char * currentPacket = (char *) calloc(PACKET_SIZE+1,1);
    printf("**************** %d ************ \n",dataSize);
    int counter = 1;
    int done=0;
    while(counter * PACKET_SIZE < dataSize)
    {
        memcpy(currentPacket,data,PACKET_SIZE);
        if(sendPacketWrapper(currentPacket,sd,PACKET_SIZE,server) == -1)
        {
          printf("Eroare la trimiterea unui pachet \n");
          return;
        }
        //printf("Am trimis pachetul %d \n",counter);
        data += PACKET_SIZE;
        counter ++;

        /* astept ACK*/
        if(receivePacketWrapper(sd,3,server) == 0) //daca am primit ack continui
          continue;
    }

    data -= PACKET_SIZE;
   // printf("am parasit bucla");
   // fflush(stdout);

    memcpy(currentPacket,data,dataSize-(counter-1)*PACKET_SIZE);
     if(sendPacketWrapper(currentPacket,sd,PACKET_SIZE,server) == -1)
        {
          printf("Eroare la trimiterea unui pachet \n");
          return;
        }
    strcpy(currentPacket,"STOP");

     if(sendPacketWrapper(currentPacket,sd,PACKET_SIZE,server) == -1)
    {
      printf("Eroare la trimiterea pachetului de STOP \n");
      return;
    }
    printf("AM intrat de %d ori \n",counter);
  }
  close(sd);
}

int main (int argc, char *argv[])
{
  int sd;		
  struct sockaddr_in server;	 
  char msg[100];	
  int msglen=0,length=0;

  /* exista toate argumentele in linia de comanda? */
  if (argc != 3)
    {
      printf ("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
      return -1;
    }

  port = atoi (argv[2]);

  if ((sd = socket (AF_INET, SOCK_DGRAM, 0)) == -1)
    {
      perror ("Eroare la socket().\n");
      return errno;
    }

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr(argv[1]);
  server.sin_port = htons (port);

  //citesc fisierul
  readFile();

  /* citirea mesajului */
  bzero (msg, 5);
  printf ("Select the mechanism used \n");
  fflush (stdout);
  read (0, msg, 5);
  printf("am citit de la key %s \n ",msg);

  if (strcmp(msg,"STRMG") == 0)
      doStreaming(sd,server);
  else if (strcmp(msg,"STAWT") == 0)
          doStopAndWait(sd,server);
  

/*
  length=sizeof(server);
  if (sendto (sd, msg, 100,0, (struct sockaddr*)&server, length) <= 0)
    {
      perror ("[client]Eroare la sendto() spre server.\n");
      return errno;
    }

    if ( (msglen=recvfrom (sd, msg, 100,0,(struct sockaddr*)&server, &length)) < 0)
    {
      perror ("[client]Eroare la recvfrom() de la server.\n");
      return errno;
    }
  /* afisam mesajul primit */

  close (sd);
}

