/*
 * list.h
 *
 *  Created on: 19 Oct 2010
 *      Author: James
 */

#ifndef LIST_H_
#define LIST_H_

//Circular linked list implementation
// Based on Linux linked lists
//

//List item type
// Use in a struct so it can be added to a list
typedef struct STagListItem
{
	struct STagListItem * next;
	struct STagListItem * prev;

} ListItem;

//Initialisers
#define LIST_SINIT(name) { &(name), &(name) }

static inline void ListInit(ListItem * list)
{
	list->next = list;
	list->prev = list;
}

//Adding
// Adds toAdd after list
//  toAdd does not need to be initialised
static inline void ListAddAfter(ListItem * list, ListItem * toAdd)
{
	list->next->prev = toAdd;
	toAdd->next = list->next;
	toAdd->prev = list;
	list->next = toAdd;
}

// Adds toAdd before list
//  toAdd does not need to be initialised
static inline void ListAddBefore(ListItem * list, ListItem * toAdd)
{
	list->prev = toAdd;
	toAdd->next = list;
	toAdd->prev = list->prev;
	list->prev->next = toAdd;
}

//Detaches the list item from the rest of the list
// This leaves the list in an undefined state
// Must be reinitialised before reuse
static inline void ListDetach(ListItem * list)
{
	list->next->prev = list->prev;
	list->prev->next = list->next;
}

//Detaches the list item from the rest of the list
// This "forms" a new list with it as the only item
static inline void ListDetachInit(ListItem * list)
{
	ListDetach(list);
	list->next = list;
	list->prev = list;
}

//Gets the data from a list item
#define LIST_DATA(list, type, member) \
		(type *) ((char *) (list) - ((size_t) &((type *) 0)->member))

//Test if a list HEAD is empty
static inline bool ListEmpty(ListItem * item)
{
	return item == item->next;
}

//List Iteration
#define LIST_FOREACH(listHead, dataVar, type, member) \
	for(type * dataVar = LIST_DATA((listHead)->next); \
		listHead != &(dataVar->member), \
		dataVar = LIST_DATA(dataVar->member->next))

#endif /* LIST_H_ */
