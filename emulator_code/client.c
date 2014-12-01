#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 
#include <sys/time.h>

int main(int argc, char *argv[])
{
    int sockfd = 0;
    double n = 0;
    char recvBuff[4096*4];
    struct sockaddr_in serv_addr; 

    timeval t1, t2;
    double elapsedTime;


    if(argc != 2)
    {
        printf("\n Usage: %s <ip of server> \n",argv[0]);
        return 1;
    } 

    memset(recvBuff, '0',sizeof(recvBuff));
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    } 

    memset(&serv_addr, '0', sizeof(serv_addr)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(1234); 

    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    } 

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       return 1;
    } 
    double count =0;
    gettimeofday(&t1, NULL);
    while ( (n = read(sockfd, recvBuff, sizeof(recvBuff))) > 0)
    {
        count+=n;
    } 
    gettimeofday(&t2, NULL);
    elapsedTime =(t2.tv_sec - t1.tv_sec)*1000000.0;
    elapsedTime +=(t2.tv_usec-t1.tv_usec);
    double bandwidth = (count * 8.0)/(1.024*1.024*elapsedTime);
    printf("\ntotal bytes received is %f and bandwidth is %f\n", count, bandwidth);

    if(n < 0)
    {
        printf("\n Read error \n");
    } 

    return 0;
}