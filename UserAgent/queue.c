#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <time.h> 
#include <string.h>    //memset

typedef struct {
	struct item * next;
	char * value; 
} item;

typedef struct {
	item * head;
	item * tail;
	int size;
} queue;


void queue_init(queue * Q){

	Q->head = NULL;	
	Q->tail = NULL;
	Q->size = 0;
}

void enqueue(queue * Q, char * value){
	void * ptr = malloc(sizeof (item));
	item * newitem = (item *) ptr;
	newitem->value = value;
	if (Q->head ==NULL){
		Q->tail = newitem;
		Q->head = newitem;
		newitem->next = NULL;
		Q->size = 1;
	} else{
		newitem->next = NULL;
		Q->tail->next = newitem;
		Q->tail = newitem;
		Q->size +=1;
	}
}
char * dequeue(queue * Q){
	item * topitem = Q->head;
	if (Q->size ==0)
		return NULL;
	else if (Q->size ==1){
		char * retv = topitem->value;
		free(topitem);
		Q->size = 0;
		Q->head = NULL;
		Q->tail = NULL;
		return retv;
	} else {
		char * retv = topitem->value;
		Q->head = Q->head->next;
		Q->size -= 1;
		free(topitem);
		return retv;
	}
}

int main(){
	queue bufferqueue ;
	char * value1 = "aaaa";
	char * value2 = "nnn";
	char * value3 = "hahah";
	queue_init(&bufferqueue);
	enqueue(&bufferqueue, value1);
	printf("%s\n", dequeue(&bufferqueue));
	
	enqueue(&bufferqueue, value1);

	enqueue(&bufferqueue, value2);

	enqueue(&bufferqueue, value3);
	printf("%s\n", dequeue(&bufferqueue));
	printf("%s\n", dequeue(&bufferqueue));
	printf("%s\n", dequeue(&bufferqueue));




}