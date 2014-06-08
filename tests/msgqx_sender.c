/*
 * Takes four interger parameters: the number of message to send (multiple of 
 * three), the starting value to send, whether to send the termination message 
 * to receiver (0/1), seconds to sleep between each send
 */

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <unistd.h>

#include <messageQx.h>
#include <common_toolx.h>

#include "msgqx_test.h"

int main(int argc, char **argv)
{

	int ret_val;
	void * h;
	int size;
	int i;

	int qlen;
	struct msgqx_data d;
	int stop;
	int sleept;

	qlen = atoi(argv[1]);

	d.val = atoi(argv[2]);
	d.pid = getpid();
	d.stop = 0;

	stop = atoi(argv[3]);
	sleept = atoi(argv[4]);

	printf("New sender pid %d from %d\n", d.pid, d.val);
	
	ret_val = msgqx_open(QNAME, &h, &size);
	if(ret_val != 0){
		printf("sender %d: error open %d\n", d.pid, ret_val);
		return 1;
	}
	if(size != sizeof(struct msgqx_data)){
		printf("sender %d: wrong data size %d\n", d.pid, size);
		return 2;
	}

	for(i = 0; i< qlen/3; i++){
		ret_val = msgqx_send(h, (void*)&d);
		if(ret_val != 0){
			printf("sender %d: error blocked send %d\n", d.pid,
			       ret_val);
			return 1;
		}
		d.val++;
		sleep(sleept);
	}

	i = 0;
	while(i < qlen/3){
		ret_val = msgqx_trysend(h, (void*)&d);
		if(ret_val != 4 && ret_val != 0){
			printf("sender %d: error try send %d\n", d.pid,
			       ret_val);
			return 1;
		}
		else if(ret_val == 0){
			d.val++;
			i++;
			sleep(sleept);
		}
		else
			printf("sender %d: retrying try send value %d\n", d.pid,
			       d.val);
	}

	i = 0;
	while(i < qlen/3){
		ret_val = msgqx_timedsend(h, (void*)&d, 1, 1);
		if(ret_val != 5 && ret_val != 0){
			printf("sender %d: error timed send %d\n", d.pid,
			       ret_val);
			return 1;
		}
		else if(ret_val == 0){
			d.val++;
			i++;
			sleep(sleept);
		}
		else
			printf("sender %d: retrying timed send value %d\n", 
			       d.pid, d.val);
	}


	if(stop){
		d.stop = 1;
		printf("send message to terminated\n");
		ret_val = msgqx_send(h, (void*)&d);
		if(ret_val != 0){
			printf("sender %d: error blocked send %d\n", d.pid, 
			       ret_val);
			return 1;
		}
	}

	ret_val = msgqx_close(h);
	if(ret_val != 0){
		printf("sender %d: error closing message %d\n", d.pid, ret_val);
		return 1;
	}

	return 0;
}
