/*
 * An implementation of a static linked list with the capability to 
 * automatically increase size. I kept two lists in the data structure,
 * one for the data, one for the empty slots.
 * 
 * Author: Wei Wang <wwang@virginia.edu>
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "static_linked_listx.h"
#include "common_toolx.h"

/*
 * the space of the static linked list is allocated by chunks,
 * the chunk size is defined below
 */
#define STATIC_LINKED_LISTX_CHUNK_SIZE 128

/* 
 * the NULL pointer (index meaning a NULL)
 */
#define STATIC_LINKED_LISTX_NULL -1

/*
 * initialized a static linked list
 */
int static_linked_listx_init(void **l,
			     unsigned int item_size)
{
	int i;
	struct static_linked_listx *list;

	if(l == NULL || item_size == 0){
		CTX_LOGERR("wrong parameters: list (%p) and item_size (%d)\n",
			   l, item_size);
		return 1;
	}

	/*
	 * initialized the pointers and bookkeeping information
	 */
	list = (struct static_linked_listx*)
		malloc(sizeof(struct static_linked_listx));
	*l = (void*)list;
	list->item_size = item_size;
	list->size = STATIC_LINKED_LISTX_CHUNK_SIZE;
	list->head = list->tail = STATIC_LINKED_LISTX_NULL;
	list->len = 0;

	/*
	 * allocate space for items and pointers
	 */
	list->items = calloc(STATIC_LINKED_LISTX_CHUNK_SIZE, item_size);
	list->pointers = (struct sllst_pointer*)
		calloc(STATIC_LINKED_LISTX_CHUNK_SIZE, 
		       sizeof(struct sllst_pointer));

	if(list->items == NULL || list->pointers == NULL){
		CTX_LOGERR("Unable to allocate memory with error %s\n",
			   sys_errlist[errno]);
		return 2;
	}
	
	/*
	 * assign pointers for the empty list
	 */
	for(i = 0; i < STATIC_LINKED_LISTX_CHUNK_SIZE; i++){
		list->pointers[i].next = i+1;
		list->pointers[i].prev = i-1;
		list->pointers[i].has_data = 0;
	}
	list->pointers[0].prev = STATIC_LINKED_LISTX_NULL;
	list->pointers[STATIC_LINKED_LISTX_CHUNK_SIZE-1].next = 
		STATIC_LINKED_LISTX_NULL;
	list->empty_head = 0;
	list->empty_tail = STATIC_LINKED_LISTX_CHUNK_SIZE-1;

	return 0;
}

/*
 * Increase the list space of a static linked list
 * Input parameters:
 *     list: the list whose space to increase
 * Return value:
 *     0: success
 *     1: list is NULL
 *     2: memory allocation error
 */
int static_linked_listx_increase(struct static_linked_listx *list)
{
	char *new_items, *new_pointers;
	int i;

	if(list == NULL)
		return 1;
	
	/*
	 * allocate more memory
	 */
	list->size += STATIC_LINKED_LISTX_CHUNK_SIZE;
	list->items = realloc(list->items, list->size * list->item_size);
	list->pointers = realloc(list->pointers, list->size * 
				 sizeof(struct sllst_pointer));
	if(list->items == NULL || list->pointers == NULL){
		CTX_LOGERR("Unable to reallocate space for static linked list"
			   "with error (%d): %s\n", errno, sys_errlist[errno]);
		return 0;
	}
	new_items = (char*)list->items + 
		(list->size - STATIC_LINKED_LISTX_CHUNK_SIZE) * list->item_size;
	new_pointers = (char*)list->pointers + 
		(list->size - STATIC_LINKED_LISTX_CHUNK_SIZE) * 
		sizeof(struct sllst_pointer);
	memset(new_items, 0, STATIC_LINKED_LISTX_CHUNK_SIZE * list->item_size);
	memset(new_pointers, 0, STATIC_LINKED_LISTX_CHUNK_SIZE * 
	       sizeof(struct sllst_pointer));
	
	
	/*
	 * adjust empty list pointers
	 */
	for(i = list->size - STATIC_LINKED_LISTX_CHUNK_SIZE; i < list->size; 
	    i++){
		list->pointers[i].next = i + 1;
		list->pointers[i].prev = i - 1;
		list->pointers[i].has_data = 0;
	}
	if(list->empty_head == STATIC_LINKED_LISTX_NULL){
		// list is full, empty head has to reset as well
		list->empty_head = list->size - STATIC_LINKED_LISTX_CHUNK_SIZE;
		list->empty_tail = list->size - 1;
		list->pointers[list->empty_head].prev = 
			STATIC_LINKED_LISTX_NULL;
		list->pointers[list->empty_tail].next = 
			STATIC_LINKED_LISTX_NULL;
	}
	else{
		list->pointers[list->empty_tail].next = list->size - 
			STATIC_LINKED_LISTX_CHUNK_SIZE;
		list->empty_tail = list->size - 1;
		list->pointers[list->empty_tail].next = 
			STATIC_LINKED_LISTX_NULL;
	}
	
	return 0;
}

/*
 * insert a new item into the list
 */
int static_linked_listx_insert(void *l, void * item)
{
	char *dest;
	int idx;
	struct static_linked_listx *list = (struct static_linked_listx*)l;

	if(list == NULL || item == NULL){
		CTX_LOGERR("wrong parameter: list (%p) and item (%p)\n", list,
			   item);
		return 1;
	}

	/*
	 * there is no room left in the linked list
	 */
	if(list->empty_head == STATIC_LINKED_LISTX_NULL){
		CTX_DPRINTF("allocating more space\n");
		if(static_linked_listx_increase(list))
			return 2;
	}

	/*
	 * save into the first empty space
	 */
	idx = list->empty_head;
	dest = (char*)list->items + idx * list->item_size;
	CTX_DPRINTF("dest is %p, list is %p\n", dest, list->items);
	memcpy(dest, item, list->item_size);
	list->pointers[idx].has_data = 1;
	
	/*
	 * remove the first item from empty list
	 */
	list->empty_head = list->pointers[list->empty_head].next;
	if(list->empty_head != STATIC_LINKED_LISTX_NULL)
		list->pointers[list->empty_head].prev = 
			STATIC_LINKED_LISTX_NULL;
	else
		list->empty_tail = STATIC_LINKED_LISTX_NULL;

	/*
	 * insert the new item to data list
	 */
	if(list->tail != STATIC_LINKED_LISTX_NULL)
		list->pointers[list->tail].next = idx;
	list->pointers[idx].prev = list->tail;
	list->pointers[idx].next = STATIC_LINKED_LISTX_NULL;
	list->tail = idx;
	if(list->head == STATIC_LINKED_LISTX_NULL)
		list->head = idx;
	list->len++;
	
	return 0;
}

/*
 * remove an item from the list
 */
int static_linked_listx_remove(void *l, int idx)
{
	int prev, next; //previous and next item of idx
	struct static_linked_listx *list = (struct static_linked_listx*)l;

	if(list == NULL || idx >= list->size || idx < 0){
		CTX_LOGERR("wrong parameters: list (%p) and idx (%d)\n", list, 
			   idx);
		return 1;
	}

	if(!list->pointers[idx].has_data){
		CTX_LOGERR("wrong parameters: idx (%d) has no data\n", idx);
		return 1;
	}
	
	/*
	 * remove the item for data list
	 */
	prev = list->pointers[idx].prev;
	next = list->pointers[idx].next;
	
	if(idx == list->head){
		// removing the first item in the list
		list->head = next;
		if(list->head == STATIC_LINKED_LISTX_NULL)
			// this is also the last item
			list->tail = STATIC_LINKED_LISTX_NULL;
		else
			list->pointers[next].prev = STATIC_LINKED_LISTX_NULL;
	}
	else if(idx == list->tail){
		// removing the last item
		list->tail = list->pointers[idx].prev;
		list->pointers[prev].next = STATIC_LINKED_LISTX_NULL;
	}
	else{
		// removing an item inside the list
		list->pointers[prev].next = next;
		list->pointers[next].prev = prev;
	}
	list->len--;
	
	/*
	 * add the item back to the empty list
	 */
	if(list->empty_tail != STATIC_LINKED_LISTX_NULL){
		// empty list still has slots
		list->pointers[list->empty_tail].next = idx;
		list->pointers[idx].prev = list->empty_tail;
		list->pointers[idx].next = STATIC_LINKED_LISTX_NULL;
		list->empty_tail = idx;
	}
	else{
		// empty list is empty
		list->pointers[idx].prev = STATIC_LINKED_LISTX_NULL;
		list->pointers[idx].next = STATIC_LINKED_LISTX_NULL;
		list->empty_tail = list->empty_head = idx;
	}
	

	return 0;
}

/*
 * return the next item of pidx
 */
int static_linked_listx_get_next(void *l, int pidx, int *nidx, void **item)
{
	struct static_linked_listx *list = (struct static_linked_listx*)l;
	
	if(list == NULL || pidx >= list->size || pidx < 0 || item == NULL || 
	   nidx == NULL){
		CTX_LOGERR("wrong parameters: list (%p) and pidx (%d) "
			   "and item (%p) and nidx (%p)\n", 
			   list, pidx, item, nidx);
		return 1;
	}

	*nidx = STATIC_LINKED_LISTX_NULL;
	*item = NULL;

	if(!list->pointers[pidx].has_data){
		CTX_LOGERR("no item at pidx (%d)\n", pidx);
		return 2;
	}
	
	*nidx = list->pointers[pidx].next;
	if(*nidx != STATIC_LINKED_LISTX_NULL)
		*item = (void*)((char*)list->items + *nidx * list->item_size);

	return 0;
}


/*
 * return the first item
 */
int static_linked_listx_get_first(void *l, int *nidx, void **item)
{
	struct static_linked_listx *list = (struct static_linked_listx*)l;
	
	if(list == NULL || item == NULL || nidx == NULL){
		CTX_LOGERR("wrong parameters: list (%p), item (%p), and "
			   "nidx (%p)\n", list, item, nidx);
		return 1;
	}

	*nidx = list->head;
	*item = NULL;

	if(*nidx != STATIC_LINKED_LISTX_NULL)
		*item = (void*)((char*)list->items + *nidx * list->item_size);

	return 0;
}

/*
 * free a static linked list
 */
int static_linked_listx_free(void *l)
{
	struct static_linked_listx *list = (struct static_linked_listx*)l;
	
	if(list == NULL ){
		CTX_LOGERR("wrong parameters: list (%p)\n", list);
		return 1;
	}

	if(list->items != NULL)
		free(list->items);
	if(list->pointers != NULL)
		free(list->pointers);

	list->head = list->tail = STATIC_LINKED_LISTX_NULL;
	list->empty_head = list->empty_tail = STATIC_LINKED_LISTX_NULL;
	
	free(l);

	return 0;
}
