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
    int num;
    sscanf(argv[1], "%d", &num);
    printf("argument is %d\n", num);
    char sendBuff[4096*4];
    time_t ticks; 


    listenfd = socket(AF_INET, SOCK_STREAM, 0);
   int optval = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000); 

    if ( bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0 )
    {
         perror("ERROR on binding");
         exit(1);
    }


    listen(listenfd, 10); 
    int i=1, roundnumber= num*1000000;
    while(1)
    {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL); 

        //ticks = time(NULL);
        //snprintf(sendBuff, sizeof(sendBuff), "%.24s\r\n", ctime(&ticks));
        //write(connfd, sendBuff, strlen(sendBuff)); 
        //memset(sendBuff, '0', sizeof(sendBuff)); 
        for (i=1; i<roundnumber; i++)
        {
            n= write(connfd, sendBuff, strlen(sendBuff)); 
        }
        close(connfd);
        sleep(1);
     }
}
