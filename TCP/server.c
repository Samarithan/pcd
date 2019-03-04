#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define PORT 2024
#define PACKET_SIZE 30000

extern int errno;
int bytesRead;
int packetsNo;
unsigned char * receivedData;

void doStreaming(int socketDesc)
{
  int retVal;
  char * data = (char *)calloc(PACKET_SIZE+1,1);
  int counter =0 ;

  while(1)
  {
    retVal = read(socketDesc,data,PACKET_SIZE);
    if(retVal<=0)
    {
      printf("Eroare la primirea pachetelor \n ");
      fflush(stdout);
      return;
    }
    bytesRead += retVal;
    packetsNo++;
    counter ++;
   // printf("Am primit pachetul %d \n",counter);

    if(strcmp(data,"STOP") == 0) 
      {
        printf("Am primit mesajul de oprire \n");
        fflush(stdout);
        break;
       } 
  }
}

int sendAck(int socketDescriptor)
{
  int retVal;
  char ack[4]="ACK";

  retVal = write(socketDescriptor,ack,3);

  if(retVal <= 0)
  {
    return -1;
  }
  else return 0;
}

void doStopAndWait(int socketDesc)
{
  int retVal;
  char * data = (char *)calloc(PACKET_SIZE+1,1);
  int counter =0 ;

  while(1)
  {
    retVal = read(socketDesc,data,PACKET_SIZE);
    if(retVal<=0)
    {
      printf("Eroare la primirea pachetelor \n ");
      fflush(stdout);
      return;
    }

    bytesRead += retVal;
    packetsNo ++;
    counter ++;
   // printf("Am primit pachetul %d \n",counter);

    /*trimit ACK*/
    if(sendAck(socketDesc) !=0)
     {
      printf("eroare la trimiterea ACK \n");
      fflush(stdout);
      return;
     }

    if(strcmp(data,"STOP") == 0) 
      {
        printf("Am primit mesajul de oprire \n");
        fflush(stdout);
        break;
      }
  }

}

void handleOption(char * msg,int socketDesc)
{ 
  clock_t begin;
  clock_t end;
  double time_spent;

  if(strcmp(msg,"STRMG") == 0)
  {   
      begin = clock();
      doStreaming(socketDesc);
      end = clock();
      time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
      printf("TCP :: STREAMING :: %f \n ",time_spent);
      printf("%d bytes read \n",bytesRead);
      printf("%d packet received \n",packetsNo);
  }
  else if(strcmp(msg,"STAWT") == 0)
  {
    begin = clock();
    doStopAndWait(socketDesc);
    end = clock();
    printf("TCP :: STOP AND WAIT :: %f",time_spent);
    printf("%d bytes read \n",bytesRead);
    printf("%d packet received \n",packetsNo);
  }

  close(socketDesc);
}

int main ()
{
  struct sockaddr_in server;	
  struct sockaddr_in from;	
  char * msg = (char *) calloc(7,1);		
  char msgrasp[100]=" ";        
  int sd;			

  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[server]Eroare la socket().\n");
      return errno;
    }

  bzero (&server, sizeof (server));
  bzero (&from, sizeof (from));
  
    server.sin_family = AF_INET;	
    server.sin_addr.s_addr = htonl (INADDR_ANY);
    server.sin_port = htons (PORT);
  
  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
      perror ("[server]Eroare la bind().\n");
      return errno;
    }

  if (listen (sd, 1) == -1)
    {
      perror ("[server]Eroare la listen().\n");
      return errno;
    }

    int client;
    int length = sizeof (from);

    printf ("[server]Asteptam la portul %d...\n",PORT);
    fflush (stdout);

    /* acceptam un client (stare blocanta pina la realizarea conexiunii) */
    client = accept (sd, (struct sockaddr *) &from, &length);

  /* eroare la acceptarea conexiunii de la un client */
  if (client < 0)
	{
	  perror ("[server]Eroare la accept().\n");
    return 0;
	}

/* s-a realizat conexiunea, se astepta mesajul */
printf ("[server]Asteptam mesajul...\n");
fflush (stdout);
      
  /* citirea mesajului */
  if (read (client, msg, 5) <= 0)
	{
	  perror ("[server]Eroare la read() de la client.\n");
	  close (client);
	}
  else
    handleOption(msg,client);

}			

