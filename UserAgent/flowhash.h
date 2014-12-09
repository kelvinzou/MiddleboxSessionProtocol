#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <asm/types.h>

#include "hash.h"


typedef struct{
	int src_ip;
	int dst_ip;
	__u16 src_port;
	__u16 dst_port;
} flow_key;

typedef struct 
{
	flow_key key;
	int sequenceNumber;
	int acked ;
	 UT_hash_handle hh;
} flow;

flow * records = NULL;

void findItem(int src_ip, int dst_ip, __u16 src_port, __u16 dst_port, flow *  * ret_ptr) {
	flow item;
	memset(&item, 0,sizeof(flow) );
	item.key.src_ip=src_ip;
	item.key.dst_ip = dst_ip;
	item.key.dst_port = dst_port;
	item.key.src_port = src_port;
	HASH_FIND(hh, records, &item.key, sizeof(flow_key), * ret_ptr);
	flow * ptr= *ret_ptr;
	
	return;
}

void addItem(int src_ip, int dst_ip, __u16 src_port, __u16 dst_port, int sequenceNumber){


	flow * ret_ptr = NULL;
	findItem(src_ip, dst_ip,src_port,  dst_port, &ret_ptr );
	//add new item
	if (ret_ptr ==NULL){
		printf("New item!\n");
		flow * ItemPtr = (flow * ) malloc(sizeof(flow));
		ItemPtr->key.src_ip=src_ip;
		ItemPtr->key.dst_ip = dst_ip;
		ItemPtr->key.dst_port = dst_port;
		ItemPtr->key.src_port = src_port;
		ItemPtr->sequenceNumber = sequenceNumber;
		ItemPtr->acked = 0;
		HASH_ADD(hh, records, key, sizeof(flow_key), ItemPtr);
	} else{
		//update item
		printf("update item!\n");
		ret_ptr->acked = 0;
		ret_ptr->sequenceNumber = sequenceNumber;
	}
	return;
}	


void clearHash(){
	flow * p, *tmp;
	 HASH_ITER(hh, records, p, tmp) {
      HASH_DEL(records, p);
      free(p);
    }
}



/*
int main(int argc, char *argv[])
{
	addItem(12,134,14,53,2);
	addItem(34,134,14,53,2);
	addItem(12,23,14,53,2);
	flow * retv = NULL;
	findItem(12,134,14,53,&retv);
	if(retv) printf("item sequenceNumber is %d\n",retv->sequenceNumber);
	addItem(12,134,14,53,32);
	findItem(12,134,14,53,&retv);
	if(retv) printf("item sequenceNumber is %d\n",retv->sequenceNumber);
	
	clearHash();
}
*/
