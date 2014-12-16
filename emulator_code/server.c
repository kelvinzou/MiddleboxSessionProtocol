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

int main(int argc, char *argv[])
{

    int listenfd = 0, connfd = 0, n=0;
    struct sockaddr_in serv_addr; 

   
    time_t ticks; 

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int optval = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5001); 

    if ( bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0 )
    {
         perror("ERROR on binding");
         exit(1);
    }

    listen(listenfd, 10); 
    int blocksize = 100000000;
    blocksize = 10000;
    while(1)
    {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL); 
 	void * sendBuff = malloc(blocksize*sizeof(char));
        //ticks = time(NULL);
	int i =0;	
	memset((char *)sendBuff, '0', blocksize*sizeof(char) ); 
		
	for (i=0; i<30; i++){
	 int n = write(connfd, sendBuff, blocksize*sizeof(char) ); 
	  sleep(1);
	  if (n<0){
	    printf("\n Write error \n");
        printf("Error with write() is %s!\n", strerror(errno));
	  }
	}
        //snprintf(sendBuff, sizeof(sendBuff), "%.24s\r\n", ctime(&ticks));
        //write(connfd, sendBuff, strlen(sendBuff)); 
        //
	free(sendBuff);
        sleep(1);
        close(connfd);
       
     }
	
}
