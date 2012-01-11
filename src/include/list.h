/*
 * list.h
 *
 *  Copyright 2012 James Cowgill
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  Created on: 8 Jan 2012
 *      Author: James
 */

#ifndef __LIST_H
#define __LIST_H

/*
 * This is a reimplementation of the linux linked list functions
 * (only the interfaces are the same)
 */

#include "chaff.h"

//List head
// Use this at the list's head and where you want to place an entry to the list
typedef struct ListHeadStruct
{
	struct ListHeadStruct * next;
	struct ListHeadStruct * prev;

} ListHead;

//List Head Initializers
// Sets up the list head into its own empty list

// Inline structure initializer - name should be the full name of the list head
#define LIST_INLINE_INIT(name) { &(name), &(name) }

// Initializes the given list head
static inline void ListHeadInit(ListHead * head)
{
	head->next = head;
	head->prev = head;
}

//Returns true if the list with the given head is empty
static inline bool ListEmpty(ListHead * head)
{
	return head->next == head;
}

//Manipulators
// Adds newItem before item in the list
// The newItem must be initialized
static inline void ListAddBefore(ListHead * newItem, ListHead * item)
{
	newItem->next = item;
	newItem->prev = item->prev;
	item->prev->next = newItem;
	item->prev = newItem;
}

// Adds newItem after item in the list
// The newItem must be initialized
static inline void ListAddAfter(ListHead * newItem, ListHead * item)
{
	newItem->prev = item;
	newItem->next = item->next;
	item->next->prev = newItem;
	item->next = newItem;
}

// Adds newItem at the front of the list (as the first element)
// The newItem must be initialized
#define ListHeadAddFirst(newItem, head) ListAddAfter((newItem), (head))


// Adds newItem at the back of the list (as the last element)
// The list in newItem must be initialized
#define ListHeadAddLast(newItem, head) ListAddBefore((newItem), (head))

// Deletes the given item from a list
//  The list MUST be re-initialized before being used again
static inline void ListDelete(ListHead * item)
{
	item->prev->next = item->next;
	item->next->prev = item->prev;
}

// Deletes the given item from a list and then re-initializes the item
static inline void ListDeleteInit(ListHead * item)
{
	ListDelete(item);
	ListHeadInit(item);
}

//Converts a list head pointer into a pointer to the containing structure
#define ListEntry(item, type, member) \
	((type *) ((char *) (item) - offsetof(type, member)))

//List Iterators
// Iterates over the list given by head using the variable var as the iterator variable
#define ListForEach(var, head, member) \
	for(var = (head)->member.next; var != (head)->member; var = var->next)

// Iterates over the list given by head using the variable var as the iterator variable
//  This is safe to use while removing the current variable
#define ListForEachSafe(var, tmpVar, head, member) \
	for(var = (head)->member.next, tmpVar = var->next; \
		var != (head)->member; \
		var = tmpVar, tmpVar = tmpVar->next)

// Iterates over the list given by head using the variable var as the iterator variable entry
#define ListForEachEntry(var, head, member) \
	for(var = ListEntry((head)->next, typeof(*(var)), member); \
		&var->member != (head); \
		var = ListEntry(var->member.next, typeof(*(var)), member))

// Iterates over the list given by head using the variable var as the iterator variable entry
//  This is safe to use while removing the current variable
#define ListForEachEntrySafe(var, tmpVar, head, member) \
	for(var = ListEntry((head)->next, typeof(*(var)), member), \
		 tmpVar = ListEntry(var->member.next, typeof(*(var)), member); \
		&var->member != (head); \
		var = tmpVar, \
		 tmpVar = ListEntry(tmpVar->member.next, typeof(*(var)), member))

// Iterates backwards over the list given by head using the variable var as the iterator variable
#define ListForEachReverse(var, head, member) \
	for(var = (head)->member.prev; var != (head)->member; var = var->prev)

// Iterates backwards over the list given by head using the variable var as the iterator variable
//  This is safe to use while removing the current variable
#define ListForEachSafeReverse(var, tmpVar, head, member) \
	for(var = (head)->member.prev, tmpVar = var->prev; \
		var != (head)->member; \
		var = tmpVar, tmpVar = tmpVar->prev)

// Iterates backwards over the list given by head using the variable var as the iterator variable entry
#define ListForEachEntryReverse(var, head, member) \
	for(var = ListEntry((head)->prev, typeof(*(var)), member); \
		&var->member != (head); \
		var = ListEntry(var->member.prev, typeof(*(var)), member))

// Iterates backwards over the list given by head using the variable var as the iterator variable entry
//  This is safe to use while removing the current variable
#define ListForEachEntrySafeReverse(var, tmpVar, head, member) \
	for(var = ListEntry((head)->prev, typeof(*(var)), member), \
		 tmpVar = ListEntry(var->member.prev, typeof(*(var)), member); \
		&var->member != (head); \
		var = tmpVar, \
		 tmpVar = ListEntry(tmpVar->member.prev, typeof(*(var)), member))

#endif
