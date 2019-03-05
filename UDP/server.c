#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
/* portul folosit */
#define PORT 2728
#define PACKET_SIZE 30000

int bytesRead;
int packetsNo;

/* codul de eroare returnat de anumite apeluri */
extern int errno;

int sendAck(int socketDescriptor,struct sockaddr_in client)
{
  int retVal;
  char ack[4]="ACK";
  int length = sizeof(client);

  retVal = sendto(socketDescriptor, ack, 3, 0, (struct sockaddr*) &client, length);

  if(retVal <= 0)
  {
    return -1;
  }
  else return 0;
}

void doStreaming(int sd,struct sockaddr_in client)
{
  printf("Stream");
  fflush(stdout);
  int length = sizeof(client);
  int retVal;
  char * data = (char *)calloc(PACKET_SIZE+1,1);
  int counter =0 ;

  while(1)
  {

     retVal = recvfrom(sd, data, PACKET_SIZE, 0,(struct sockaddr*) &client, &length);

    if(retVal<0)
    {
      printf("Eroare la primirea pachetelor \n ");
      fflush(stdout);
      return;
    }
    bytesRead += retVal;
    packetsNo ++;
    counter ++;
    printf("Am primit pachetul %d \n",counter);

    if(strcmp(data,"STOP") == 0) 
      {
        printf("Am primit mesajul de oprire \n");
        fflush(stdout);
        break;
      } 
  }
}

void doStopAndWait(int sd,struct sockaddr_in client)
{
  printf("Stop and wAIT");
  fflush(stdout);
  int retVal;
  int length = sizeof(client);
  char * data = (char *)calloc(PACKET_SIZE+1,1);
  int counter =0 ;

  while(1)
  {
    retVal = retVal = recvfrom(sd, data, PACKET_SIZE, 0,(struct sockaddr*) &client, &length);
    if(retVal<0)
    {
      printf("Eroare la primirea pachetelor \n ");
      fflush(stdout);
    }
    bytesRead+=retVal;
    packetsNo++;
    counter ++;
    printf("Am primit pachetul %d \n",counter);

    /*trimit ACK*/
    if(sendAck(sd,client) !=0)
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

void handleOption(int socketDesc,char * msg,struct sockaddr_in client)
{ 
  clock_t begin;
  clock_t end;
  double time_spent;

   if(strcmp(msg,"STRMG") == 0)
  { 
      begin = clock();
      doStreaming(socketDesc,client);
      end = clock();
      time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
      printf("UDP :: STREAMING :: %f\n",time_spent);
      printf("%d bytes read \n",bytesRead);
      printf("%d packet received \n",packetsNo);
  }
  else if(strcmp(msg,"STAWT") == 0)
  { 
    begin = clock();
    doStopAndWait(socketDesc,client);
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("UDP :: STOP AND WAIT :: %f\n",time_spent);
    printf("%d bytes read \n",bytesRead);
    printf("%d packet received \n",packetsNo);
  }


}

int main ()
{
  struct sockaddr_in server;	// structura folosita de server
  struct sockaddr_in client;	
  char msg[100];		//mesajul primit de la client 
  char msgrasp[100]=" ";        //mesaj de raspuns pentru client
  int sd;			//descriptorul de socket 

  /* crearea unui socket */
  if ((sd = socket (AF_INET, SOCK_DGRAM, 0)) == -1)
    {
      perror ("[server]Eroare la socket().\n");
      return errno;
    }

  /* pregatirea structurilor de date */
  bzero (&server, sizeof (server));
  bzero (&client, sizeof (client));
  
  /* umplem structura folosita de server */
  /* stabilirea familiei de socket-uri */
    server.sin_family = AF_INET;	
  /* acceptam orice adresa */
    server.sin_addr.s_addr = htonl (INADDR_ANY);
  /* utilizam un port utilizator */
    server.sin_port = htons (PORT);
  
  /* atasam socketul */
  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
      perror ("[server]Eroare la bind().\n");
      return errno;
    }

  

    int msglen;
    int length = sizeof (client);

    printf ("[server]Astept la portul %d...\n",PORT);
    fflush (stdout);

    bzero (msg, 100);
      
  /* citirea mesajului primit de la client */
  if ((msglen = recvfrom(sd, msg, 100, 0,(struct sockaddr*) &client, &length)) <= 0)
	{
	  perror ("[server]Eroare la recvfrom() de la client.\n");
	  return errno;
	}
	else
  {
    printf ("[server]Mesajul a fost receptionat...%s\n", msg);
    handleOption(sd,msg,client);
  }       
      /*
  if (sendto(sd, msgrasp, 100, 0, (struct sockaddr*) &client, length) <= 0)
	{
	  perror ("[server]Eroare la sendto() catre client.\n");
	  continue;
	}
      else
	printf ("[server]Mesajul a fost trasmis cu succes.\n");
*/
}

