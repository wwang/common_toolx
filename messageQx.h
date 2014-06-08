/*
 * The header file of a general message queue.
 * 
 * Author: Wei Wang <wwang@virginia.edu>
 */

#ifndef __MESSAGE_QUEUE_X_H__
#define __MESSAGE_QUEUE_X_H__

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Create a new message queue.
 *
 * Input parameters:
 *     name: the name of message queue;
 *     size: the size of each message;
 *     len: maximum number of messages in the queue
 * Ouput parameters:
 *     handle: the handle to the message queue;
 * Return values:
 *     0: success
 *     1: failed to create receiving semaphore
 *     2: failed to create sending semaphore
 *     3: failed to create shared memory
 *     4: failed to create message queue mutex
 */
int msgqx_create(const char * name, int size, int len, void ** handle);

/*
 * Open an existing message queue.
 *
 * Input parameters:
 *     name: the name of message queue;
 * Ouput parameters:
 *     handle: the handle to the message queue;
 *     size: the size of each message (should be)
 * Return values:
 *     0: success
 *     1: failed to open receiving semaphore
 *     2: failed to open sending semaphore
 *     3: failed to open shared memory
 *     4: failed to create message queue mutex
 */
int msgqx_open(const char *name, void **handle, int *size);

/*
 * Send a message to th queue. Note that it is the sender's responsibility to 
 * make sure that the size of data is proper. 
 *
 * Input parameters:
 *     handle: the handle to the message queue
 *     data: the data to copy to the message queue.
 * Input parameters for timed send:
 *     sec: seconds to wait
 *     nsec: nanoseconds to wait 
 * Return values:
 *     0: success
 *     1: invalid parameters
 *     2: the queue is full (bug?)
 *     3: error when waiting from semaphore
 *     4: failed to acquire the message queue for try send
 *     5: failed to acquire the message queue for timed send
 *     6: failed to post received semaphore
 *     7: failed to release the message queue
 */
int msgqx_send(void *handle, void *data);
int msgqx_trysend(void *handle, void *data);
int msgqx_timedsend(void *handle, void *data, int sec, int nsec);

/*
 * Send a message to the queue. Note that it is the receiver's responsibility to
 * make sure that the size of buffer is large enough. 
 *
 * Input parameters:
 *     handle: the handle to the message queue
 *     buf: the data to receive the message 
 * Input parameters for timed receive:
 *     sec: seconds to wait
 *     nsec: nanoseconds to wait
 * Return values:
 *     0: success
 *     1: invalid parameters
 *     2: the queue is empty (bug?)
 *     3: error when waiting from semaphore
 *     4: failed to acquire the message queue for try receive
 *     5: failed to acquire the message queue for timed receive
 *     6: failed to post received semaphore
 *     7: failed to release the message queue
 */
int msgqx_receive(void *handle, void *buf);
int msgqx_tryreceive(void *handle, void *buf);
int msgqx_timedreceive(void *handle, void *buf, int sec, int nsec);

/* 
 * Close the message queue.
 * Input parameters:
 *     handle: the handle to the message queue
 * Return values:
 *     0: success
 *     other:  failed, check errno for reasons
 */
int msgqx_close(void *handle);

/* 
 * Destroy the message queue. Call close_msgqx before calling this function. The
 * will be marked to destroy, but it will only be destroyed after every user has
 * close the queue.
 * 
 * Input parameters:
 *     name: the name of the message queue
 * Return values:
 *     0: success
 *     other:  failed, check errno for reasons
 */
int msgqx_destroy(const char *name);

#ifdef __cplusplus
}
#endif

#endif
