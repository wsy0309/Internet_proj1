#include <stdio.h>

typedef struct route_t{
	unsigned int dstip;
	unsigned int dstport;
	unsigned int nextip;
	unsigned int nextport;
	unsigned int metric;	
}route_entry;
