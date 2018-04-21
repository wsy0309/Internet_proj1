
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>
#include "router.h"

#define MAX_ROUTES 5

int main(int argc, char* argv[])
{
	int port_num;
	FILE *vlinkfp = NULL;  //file
	route_entry *re = NULL; //route entry
//	hashtable_t * hashtable = ht_create(5);
	int temp[3];
	char  *ret;
	unsigned char dstip_addr[4], nextip_addr[4];

	if (argc != 2){
		printf("usage : srv port route_vlink_input\n");
//		return 0;
	}
	if ((port_num = atoi(argv[1])) == 0){
		perror("invalid port number");
		return 0;
	}
	if ((vlinkfp = fopen(argv[2], "r")) == NULL){
		printf("%d", errno);
		perror(argv[2]);
		return 0;
	}

	printf("vlink setup \n");
	printf("port num : %d\n",port_num);

	//fill route table
	for(int i = 0; MAX_ROUTES; i++){
		ret = fscanf(vlinkfp, "%u.%u.%u.%u %d %u.%u.%u.%u %d %d",
			&dstip_addr[0], &dstip_addr[1], &dstip_addr[2], &dstip_addr[3], &temp[0],
			&nextip_addr[0],&nextip_addr[1],&nextip_addr[2],&nextip_addr[3], &temp[1], &temp[2]);
		printf("dstip : %u %u %u %u\n",dstip_addr[0], dstip_addr[1],dstip_addr[2],dstip_addr[3]);
		printf("dstport : %d\n",temp[0]);
		printf("nextip : %u %u %u %u \n",nextip_addr[0],nextip_addr[1],nextip_addr[2],nextip_addr[3]);


		if (ret == EOF)
			break;
		char* result[2];
		
		re->dstip = *(int*)dstip_addr;
		re->dstport = temp[0];
		re->nextip = *(int*)nextip_addr;
		re->nextport = temp[1];
		re->metric = temp[2];

		printf("Dest %d : %d  Nexthop %d : %d  metric %d\n", re->dstip, re->dstport, re->nextip, re->nextport, re->metric);
	
	}

	fclose(vlinkfp);
	return 0;
}
