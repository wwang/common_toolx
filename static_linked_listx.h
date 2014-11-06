/*
 * An implementation of a static linked list
 * 
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#ifndef __COMMON_TOOLX_STATIC_LINKED_LISTX_H__
#define __COMMON_TOOLX_STATIC_LINKED_LISTX_H__

#ifdef __cplusplus
extern "C" {
#endif

struct sllst_pointer{
	int has_data; // whether this slot has data
	int next; // array index of next item
	int prev; // array index of previous item
};

struct static_linked_listx{
	unsigned int item_size; // the size of the item
	void * items; // the array of the data items
	struct sllst_pointer * pointers; // the array of the pointers;
	int len; // the number of items
	int size; // the size of the items array
	int head; // the index of the first data
	int tail; // the index of the last data
	int empty_head; // the index of the first empty item
	int empty_tail; // the index of the last empty item
};

/*
 * Initialized a static linked list.
 * Input parameters:
 *     list: the handle to the list
 *     item_size: the size of each item
 * Return values:
 *     0: success
 *     1: wrong parameter, list is NULL and/or item_size is 0
 *     2: unable to allocate space
 */
int static_linked_listx_init(void **list,
			     unsigned int item_size);

/*
 * insert an item into the linked list
 * Input parameters:
 *     list: the list to which insert
 *     item: the item to be inserted
 * Return values:
 *     0: success
 *     1: wrong parameter, list and/or item is NULL
 *     2: unable to increase list space
 */
int static_linked_listx_insert(void *list, void * item);

/*
 * Remove an item into the linked list
 * Input parameters:
 *     list: the list to which insert
 *     idx: the index of the item
 * Return values:
 *     0: success
 *     1: wrong parameter, list is NULL or idx is invalid index
 *     2: unable to increase list space
 */
int static_linked_listx_remove(void *list, int idx);

/*
 * Return the first or next item of pidx from the list
 * Input parameters:
 *     list: the static linked list
 *     pidx: the next of which is returned
 * Output parameters:
 *     nidx: the index of the requested item,
 *     item: the address of the requested item, NULL mean no item
 * Return value:
 *     0: success
 *     1: wrong parameters
 *     2: pidx has no item
 */

int static_linked_listx_get_first(void *list, 
				  int *nidx, void **item);
int static_linked_listx_get_next(void *list, 
				 int pidx, int *nidx, void **item);

/*
 * free a static linked list.
 * Input parameters:
 *     list: the list to initialized, caller should allocate space for it
 * Return values:
 *     0: success
 *     1: wrong parameter, list
 */
int static_linked_listx_free(void *list);
	
#ifdef __cplusplus
}
#endif

#endif 
