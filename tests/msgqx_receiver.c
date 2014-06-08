 /*
 * Takes four interger parameters: whether to create the queue, the quene 
 * length, the wait type (blocked:0, try:1, timed:2), seconds to sleep 
 * between receives.
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
	int create;
	int qlen;
	void * h;
	int size = sizeof(struct msgqx_data);
	pid_t pid;
	int wait_type;
	struct msgqx_data d;
	int sleept;

	create = atoi(argv[1]);
	qlen = atoi(argv[2]);
	pid = getpid();
	wait_type = atoi(argv[3]);
	sleept = atoi(argv[4]);

	printf("New receiver with pid %d\n", pid);

	if(create)
		ret_val = msgqx_create(QNAME, size, qlen, 
				       &h);
	else
		ret_val = msgqx_open(QNAME, &h, &size);

	if(ret_val != 0){
		printf("receiver %d: error open %d\n", pid, ret_val);
		goto error;
	}
	if(size != sizeof(struct msgqx_data)){
		printf("receiver %d: wrong data size", pid);
		goto error;
	}

	while(1){
		switch(wait_type){
		case 0:
			ret_val = msgqx_receive(h, (void*)&d);
			break;
		case 1:
			ret_val = msgqx_tryreceive(h, (void*)&d);
			break;
		case 2:
			ret_val = msgqx_timedreceive(h, (void*)&d, 3, 1);
			break;
		}

		if(ret_val == 0){
			printf("receiver %d new message: from %d, val %d, stop "
			       "%d\n", pid, d.pid, d.val, d.stop);

			if(d.stop)
				break;
			sleep(sleept);
		}
		else if(ret_val == 4 && wait_type == 1){
			printf("receiver %d try received to retry\n", pid);
		}    
		else if(ret_val == 5 && wait_type == 2){
			printf("receiver %d timed received to retry\n", pid);
		}
		else{
			printf("receiver %d failed to receive %d\n", pid, 
			       ret_val);
			goto error;
		}
	}

 error:
	msgqx_close(h);
	if(create)
		msgqx_destroy(QNAME);

	return ret_val;
}
