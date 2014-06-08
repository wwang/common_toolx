/*
 * Implementation of the messageQx. This is a typical implementation of the 
 * producer and consumer problem using semaphores and shared memory.
 * 
 * Author: Wei Wang <wwang@virginia.edu>
 */

#include <stdlib.h>
#include <stdio.h>

// for semaphores and shared memory
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

// for error reporting
#include <string.h>
#include <errno.h>

#include "common_toolx.h"
#include "messageQx.h"

// the structure of the message queue
typedef struct _msgqx_queue{
	int msg_size; // the size of each message
	int qlen; // the length of the queue; the queue is implemented as a
	          // ring buffer
	int first; // the index of first message
	int msg_cnt; // the number of messages in the queue
	unsigned char queue; // beginning of the memory queue
}msgqx_q;

// the handle to the message queue, works like an class object pointer
typedef struct _msgqx_handle{
	sem_t *has_msg; // semaphore for receiving message
	sem_t *has_slot;  // semaphore for sending message 
	sem_t *mutex; // mutex for protecting the message queue
	int shm_fd; // shared memory file descriptor
	msgqx_q *mem; // pointer to the shared memory
}msgqx_h;


// enumerate of the object types
typedef enum _msgqx_obj_types{
	recv_sem,
	send_sem,
	mutex_sem,
	msgq_shm,
}msgqx_ty;

#define MSGQX_NAME_PREFIX "MSGQXPPRE"
#define MSGQX_RECV_SEM_POSTFIX 'r'
#define MSGQX_SEND_SEM_POSTFIX 's'
#define MSGQX_MUTEX_SEM_POSTFIX 'm'
#define MSGQX_MSGQ_MEM_POSTFIX 'q'
#define NAME_BUFFER_SIZE 64

static int _msgqx_get_obj_name(char * buf, const char * name, msgqx_ty type)
{
	char postfix;
	char shm_slash[2] = {0,0};

	switch(type){
	case recv_sem:
		postfix = MSGQX_RECV_SEM_POSTFIX;
		break;
	case send_sem:
		postfix = MSGQX_SEND_SEM_POSTFIX;
		break;
	case mutex_sem:
		postfix = MSGQX_MUTEX_SEM_POSTFIX;
		break;
	case msgq_shm:
		postfix = MSGQX_MSGQ_MEM_POSTFIX;
		shm_slash[0]='/';
		break;
	default:
		postfix = 'x';
	}

	sprintf(buf, "%s%s_%c_%s", shm_slash, MSGQX_NAME_PREFIX,
		postfix, name);

	return 0;
}

static void _init_msgqx_handle(msgqx_h *handle)
{
	handle->has_msg = handle->has_slot = handle->mutex = SEM_FAILED;
	handle->mem = MAP_FAILED;
	handle->shm_fd = -1;

	return;
}

static void _init_msgqx_q(msgqx_q *q, int msg_size, int qlen)
{
	q->msg_size = msg_size;
	q->qlen = qlen;
	q->first = q->msg_cnt = 0;

	return;
}

// generic function for creating semaphores
// boolx new indicates whether to create a new semaphore
// the int val is the initial value of the semaphore
// the new semaphore is returned with sem_t ** sem
// return 0 on success;
static int _msgqx_open_sem(const char *name, boolx new, int val, sem_t ** sem)
{
	int flags = 0;
	int ret_val = 0;

	if(new)
		flags |= O_CREAT | O_EXCL;
	*sem = sem_open(name, flags, S_IRUSR | S_IWUSR, val);
	
	if(*sem == SEM_FAILED){
		CTX_DPRINTF("Cannot open semaphore %s for: "
			    "%s\n", name, strerror(errno));
		ret_val = 1;
	}
	
	return ret_val;
}

// generic function for opening shared memory
// return 0 on success
int _msgqx_open_shm(const char *name, boolx new, int size, int *shm_fd,
			   void **mem)
{
	int flags = O_RDWR;
	int ret_val = 0;
	
	if(new)
		flags |= O_CREAT | O_EXCL;

	// open the shared memory
	*shm_fd = shm_open(name, flags, S_IRUSR | S_IWUSR);
	if(*shm_fd == -1){
		CTX_DPRINTF("Cannot create shared memory %s: %s\n", name, 
			    strerror(errno));
		return 1;
	}

	// get the correct size of the shared memory
	if(new){
		// new shared memory, we need to change its size
		ret_val = ftruncate(*shm_fd, size);
		if(ret_val != 0){
			CTX_DPRINTF("Cannot re-size shared memory %s: %s\n", 
				    name, strerror(errno));
			return 2;
		}
	}
	else{
		// existing shared memory, we need to get its size
		struct stat s;
		ret_val = fstat(*shm_fd, &s);
		if(ret_val != 0){
			CTX_DPRINTF("Cannot fstat shared memory %s: %s\n", 
				    name, strerror(errno));
			return 3;
		}
		size = s.st_size;
	}	

	// map the shared memory
	*mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, *shm_fd, 0);
	if(*mem == MAP_FAILED){
		CTX_DPRINTF("Cannot map shared memory %s: %s\n", name, 
			    strerror(errno));
		return 4;
	}
	
	return 0;
}
			   
int msgqx_create(const char *name, int size, int len, void **h)
{
	int ret_val = 0;
	msgqx_h *handle;
	char name_buf[NAME_BUFFER_SIZE];
	
	// create the handle
	handle = (msgqx_h *)calloc(1, sizeof(msgqx_h));
	_init_msgqx_handle(handle);
	*h = (void*)handle;
	
	// open the receiving semaphore
	_msgqx_get_obj_name(name_buf, name, recv_sem);
	ret_val = _msgqx_open_sem(name_buf, truex, 0, &handle->has_msg);
	if(ret_val != 0){
		ret_val = 1;
		goto error;
	}

	// open the sending semaphore
	_msgqx_get_obj_name(name_buf, name, send_sem);
	ret_val = _msgqx_open_sem(name_buf, truex, len, &handle->has_slot);
	if(ret_val != 0){
		ret_val = 2;
		goto error;
	}

	// open the mutex that protects the message queue
	// note that the mutex is opened with value 0 because we need
	// to initialize the share memory first
	_msgqx_get_obj_name(name_buf, name, mutex_sem);
	ret_val = _msgqx_open_sem(name_buf, truex, 0, &handle->mutex);
	if(ret_val != 0){
		ret_val = 4;
		goto error;
	}

	// open the shared memory
	_msgqx_get_obj_name(name_buf, name, msgq_shm);
	ret_val = _msgqx_open_shm(name_buf, truex, sizeof(msgqx_q) + size * len,
				  &handle->shm_fd, (void**)&handle->mem);
	if(ret_val != 0){
		ret_val = 3;
		goto error;
	}

	// initialize the message queue
	_init_msgqx_q(handle->mem, size, len);
	// release the message queue, should have no error here
	sem_post(handle->mutex);
	
	return 0;
	
 error:
	msgqx_close(handle);
	msgqx_destroy(name);
	*h = NULL;
	return ret_val;
}

int msgqx_open(const char *name, void **h, int *size)
{
	int ret_val = 0;
	msgqx_h *handle;
	char name_buf[NAME_BUFFER_SIZE];
	
	// create the handle
	handle = (msgqx_h *)calloc(1, sizeof(msgqx_h));
	_init_msgqx_handle(handle);
	*h = (void*)handle;
	
	// open the receiving semaphore
	_msgqx_get_obj_name(name_buf, name, recv_sem);
	ret_val = _msgqx_open_sem(name_buf, falsex, 0, &handle->has_msg);
	if(ret_val != 0){
		ret_val = 1;
		goto error;
	}

	// open the sending semaphore
	_msgqx_get_obj_name(name_buf, name, send_sem);
	ret_val = _msgqx_open_sem(name_buf, falsex, 0, &handle->has_slot);
	if(ret_val != 0){
		ret_val = 2;
		goto error;
	}

	// open the mutex that protects the message queue
	_msgqx_get_obj_name(name_buf, name, mutex_sem);
	ret_val = _msgqx_open_sem(name_buf, falsex, 0, &handle->mutex);
	if(ret_val != 0){
		ret_val = 4;
		goto error;
	}

	// open the shared memory
	_msgqx_get_obj_name(name_buf, name, msgq_shm);
	ret_val = _msgqx_open_shm(name_buf, falsex, 0, &handle->shm_fd, 
				  (void**)&handle->mem);
	if(ret_val != 0){
		ret_val = 3;
		goto error;
	}
	*size = handle->mem->msg_size;

	return 0;
 error:
	msgqx_close(handle);
	*size = 0;
	*h = NULL;
	return ret_val;
}


// parameter check for send/recevie message
static inline int _msgqx_param_check(msgqx_h *h, void *buf)
{
	if(h == NULL || 
	   buf == NULL || 
	   h->mem == NULL || 
	   h->has_msg == SEM_FAILED || h->has_msg == NULL ||
	   h->has_slot == SEM_FAILED || h->has_slot == NULL ||
	   h->mutex == SEM_FAILED || h->mutex == NULL)
		return 1;
	else
		return 0;
}

typedef enum _msgqx_wait_type{
	blockedwait,
	trywait,
	timedwait
}msgqx_wty;

// a small function that computes the differences of two struct timespec
// data. return 1 if the diff is smaller than 0.
static int _msgqx_time_diff(struct timespec *s, struct timespec *e, 
				  int *sec, int *nsec)
{
	long long ns_start, ns_end, diff;
	
	ns_start = s->tv_sec * 1000000000 + s->tv_nsec;
	ns_end = e->tv_sec * 1000000000 + e->tv_nsec;

	diff = ns_end - ns_start;
	*sec = diff / 1000000000;
	*nsec = diff % 1000000000;

	return (diff < 0);
}

// a small function that adds seconds and nanoseconds to a struct timespec
// data. If the sum is smaller than zero, then do nothing
static void _msgqx_time_add(struct timespec *s, int sec, int nsec)
{
	long long ns;
	
	ns= (s->tv_sec+sec) * 1000000000 + s->tv_nsec + nsec;
	
	if(ns <= 0)
		return;

	s->tv_sec = ns / 1000000000;
	s->tv_nsec = ns % 1000000000;

	return;
}

// generic interface for a semaphore wait
// can be a blocked wait, a try wait or a timed wait
// return 0 if semaphore acquired;
// return 4 if try-wait failed to acquired immediately
// return 5 if timed-wait failed to acquired within time span.
// return 3 if other errors occur
static int _msgqx_wait_sem(sem_t *sem, msgqx_wty wait_type, int *sec, int *nsec)
{
	int ret_val = 0;
	struct timespec ts, te;

	switch(wait_type){
	case blockedwait:
		ret_val = sem_wait(sem);
		break;
	case trywait:
		ret_val = sem_trywait(sem);
		break;
	case timedwait:
		clock_gettime(CLOCK_REALTIME, &ts);
		_msgqx_time_add(&te, *sec, *nsec);

		ret_val = sem_timedwait(sem, &te);

		clock_gettime(CLOCK_REALTIME, &te);
		_msgqx_time_diff(&ts, &te, sec, nsec);
		break;
	default:
		break; // not reached
	}

	if(ret_val == 0)
		return ret_val;
	else if(wait_type == trywait && errno == EAGAIN)
		return 4;
	else if(wait_type == timedwait && errno == ETIMEDOUT)
		return 5;
	else{
		CTX_DPRINTF("Failed when waiting for a semaphore: %s\n",
			    strerror(errno));
		return 3;
	}
}


// generic interface for a semaphore post
// return 0 on success;
// return 1 if failed;
static int _msgqx_post_sem(sem_t * sem)
{
	if(sem_post(sem)){
		CTX_DPRINTF("Failed when posting a semaphore: %s\n",
			    strerror(errno));
		return 1;
	}

	return 0;
}

// copy the message to the message queue, always assume parameters are valid
static int _msgqx_put_msg(msgqx_h *h, void *data)
{
	unsigned char *p;
	int idx;
	
	if(h->mem->msg_cnt >= h->mem->qlen){
		CTX_DPRINTF("Strange, the queue is full\n");
		return 2;
	}
	// locate the beginning of the slot
	idx = (h->mem->first + h->mem->msg_cnt) % h->mem->qlen;
	p = &h->mem->queue + h->mem->msg_size * idx;
	// copy the data
	memcpy((void*)p, data, h->mem->msg_size);
	h->mem->msg_cnt++;
	
	return 0;
}

// generic interface for sending message
static int _msgqx_send(msgqx_h *h, void *data, msgqx_wty wait_type, int sec, 
		       int nsec)
{
	int ret_val;
	// check the parameters
	if(_msgqx_param_check(h, data))
		return 1;
	
	// wait for empty slot
	ret_val = _msgqx_wait_sem(h->has_slot, wait_type, &sec, &nsec);
	if(ret_val)
		return ret_val;
	// wait for the message queue
	ret_val = _msgqx_wait_sem(h->mutex, wait_type, &sec, &nsec);
	if(ret_val)
		return ret_val;
	// copy the message to the queue
	ret_val = _msgqx_put_msg(h, data);
	if(ret_val)
		return 2;

	// release the message queue
	ret_val = _msgqx_post_sem(h->mutex);
	if(ret_val)
		return 7;
	
	// post the receiving semaphore
	ret_val = _msgqx_post_sem(h->has_msg);
	if(ret_val)
		return 6;

	return 0;
}

int msgqx_send(void *handle, void *data)
{
	return _msgqx_send((msgqx_h*)handle, data, blockedwait, 0, 0);
}

int msgqx_trysend(void *handle, void *data)
{
	return _msgqx_send((msgqx_h*)handle, data, trywait, 0, 0);
}

int msgqx_timedsend(void *handle, void *data, int sec, int nsec)
{
	return _msgqx_send((msgqx_h*)handle, data, timedwait, sec, nsec);
}

// copy the message from the message queue, always assume parameters are valid
static int _msgqx_get_msg(msgqx_h *h, void *buf)
{
	unsigned char *p;
	
	if(h->mem->msg_cnt <= 0){
		CTX_DPRINTF("Strange, the queue is empty\n");
		return 2;
	}
	// locate the first message
	p = &h->mem->queue + h->mem->msg_size * h->mem->first;
	// copy the data
	memcpy((void*)buf, p, h->mem->msg_size);
	// update the first message index
	h->mem->first++;
	h->mem->first %= h->mem->qlen;
	h->mem->msg_cnt--;
	
	return 0;
}


// generic interface for receiving message
static int _msgqx_receive(msgqx_h *h, void *buf, msgqx_wty wait_type, int sec, 
		       int nsec)
{
	int ret_val;
	// check the parameters
	if(_msgqx_param_check(h, buf))
		return 1;
	
	// wait for new message
	ret_val = _msgqx_wait_sem(h->has_msg, wait_type, &sec, &nsec);
	if(ret_val)
		return ret_val;
	// wait for the message queue
	ret_val = _msgqx_wait_sem(h->mutex, wait_type, &sec, &nsec);
	if(ret_val)
		return ret_val;
	// copy the message to the queue
	ret_val = _msgqx_get_msg(h, buf);
	if(ret_val)
		return 2;

	// release the message queue
	ret_val = _msgqx_post_sem(h->mutex);
	if(ret_val)
		return 7;
	
	// post the sending semaphore
	ret_val = _msgqx_post_sem(h->has_slot);
	if(ret_val)
		return 6;

	return 0;
}

int msgqx_receive(void *handle, void *data)
{
	return _msgqx_receive((msgqx_h*)handle, data, blockedwait, 0, 0);
}

int msgqx_tryreceive(void *handle, void *data)
{
	return _msgqx_receive((msgqx_h*)handle, data, trywait, 0, 0);
}

int msgqx_timedreceive(void *handle, void *data, int sec, int nsec)
{
	return _msgqx_receive((msgqx_h*)handle, data, timedwait, sec, nsec);
}

// generic interface for closing a semaphore
static int _msgqx_close_sem(sem_t * sem)
{
	if(sem != SEM_FAILED && sem != NULL)
		return sem_close(sem);
	return 0;
}

// generic interface fore closing a shared memory
static int _msgqx_close_shm(void * mem, int size, int shm_fd)
{
	int ret_val = 0;
	if(mem != MAP_FAILED){
		ret_val = munmap(mem, size);
		if(ret_val != 0)
			CTX_DPRINTF("Cannot un-map shared memory: %s\n",
				    strerror(errno));
		
	}
	
	if(shm_fd != -1){
		ret_val = close(shm_fd);
		if(ret_val != 0)
			CTX_DPRINTF("Cannot close shared memory: %s\n", 
				    strerror(errno));
	}
	
	return ret_val;
}

int msgqx_close(void *handle)
{
	int size = 0, ret_val = 0;
	msgqx_h *h = handle;

	if(h == NULL)
		return 1;

	// close receiving semaphore
	ret_val |= _msgqx_close_sem(h->has_msg);
	// close sending semaphore
	ret_val |= _msgqx_close_sem(h->has_slot);
	// close message queue mutex
	ret_val |= _msgqx_close_sem(h->mutex);
	// un-map and close shared memory
	if(h->mem != NULL && h->mem != MAP_FAILED){
		size = sizeof(msgqx_q) + 
			h->mem->msg_size * h->mem->qlen;
		
		ret_val |= _msgqx_close_shm((void*)h->mem, size, h->shm_fd);
	}

	// free handle memory
	free(handle);

	return ret_val;
}

// generic interface for destroying a semaphore
static int _msgqx_destroy_sem(const char *name)
{
	int ret_val = 0;

	ret_val = sem_unlink(name);
	if(ret_val != 0)
		CTX_DPRINTF("Cannot destroy semaphore %s: %s\n", 
			    name, strerror(errno));
	
	return ret_val;
}

// generic interface for destroying a shared memory
static int _msgqx_destroy_shm(const char *name)
{
	int ret_val = 0;

	ret_val = shm_unlink(name);
	if(ret_val != 0)
		CTX_DPRINTF("Cannot destroy shared memory %s: "
			    " %s\n", name, strerror(errno));

	return ret_val;
}

// generic function for destroying semaphores and shared memory
int msgqx_destroy(const char *name)
{
	int ret_val = 0;

	char name_buf[NAME_BUFFER_SIZE];

	// destroy the receiving semaphore
	_msgqx_get_obj_name(name_buf, name, recv_sem);
	ret_val |= _msgqx_destroy_sem(name_buf);
	// destroy the sending semaphore
	_msgqx_get_obj_name(name_buf, name, send_sem);
	ret_val |= _msgqx_destroy_sem(name_buf);
	// destroy the message queue mutex
	_msgqx_get_obj_name(name_buf, name, mutex_sem);
	ret_val |= _msgqx_destroy_sem(name_buf);
	// destroy the shared memory
	_msgqx_get_obj_name(name_buf, name, msgq_shm);
	ret_val |= _msgqx_destroy_shm(name_buf);
		
	return ret_val;
}

