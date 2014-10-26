
/* Sample UDP client */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 

int main(int argc, char**argv)
{
   int sockfd,n;
   struct sockaddr_in servaddr,cliaddr;
  // char sendline[1000];
   char recvline[1500];

   if (argc < 2)
   {
      printf("usage:  udpcli <IP address>\n");
      exit(1);
   }

   sockfd=socket(AF_INET,SOCK_DGRAM,0);

   bzero(&servaddr,sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr=inet_addr(argv[1]);
   servaddr.sin_port=htons(1234);

   while (1)
   {
      sleep(2);
      char  sendline[1400] ;
	memset(sendline, 0x41,100);
	
      int round =10000;
	for (round=1; round>0; round--)
	{  sendto(sockfd,sendline,100,0,(struct sockaddr *)&servaddr,sizeof(servaddr));}
      
     if(argc==3){ 
      n=recvfrom(sockfd,recvline,1500,0,NULL,NULL);
      recvline[n]=0;
      fputs(recvline+28,stdout);
	}
   }
}
