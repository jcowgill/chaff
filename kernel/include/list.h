/**
 * @file
 * Linked list implementation
 *
 * This is a reimplementation of the Linux linked list functions
 * (only the interfaces are the same)
 *
 * @date January 2012
 * @author James Cowgill
 */

/*
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
 */

#ifndef __LIST_H
#define __LIST_H

#include "chaff.h"

/**
 * The head of a list
 *
 * This is used as the start of the list and as an item in the list
 */
typedef struct ListHead
{
	struct ListHead * next;
	struct ListHead * prev;

} ListHead;

/**
 * @name List head initializers
 * @{
 */

/**
 * Inline structure initializer
 *
 * @param name the full name of the list head
 */
#define LIST_INLINE_INIT(name) { &(name), &(name) }

/**
 * Initializes the given list head
 *
 * @param head list head to initialize
 */
static inline void ListHeadInit(ListHead * head)
{
	head->next = head;
	head->prev = head;
}

/** @} */

/**
 * Returns true if the list is empty
 *
 * The list must be initialized
 *
 * @param head list head to test
 * @return whether the list is empty
 */
static inline bool ListEmpty(ListHead * head)
{
	return head->next == head;
}

/**
 * @name List Manipulators
 * @{
 */

/**
 * Adds an item before another in the list
 *
 * @a newItem must be initialized
 *
 * @param newItem item to add before @a item
 * @param item item to insert @a newItem before
 */
static inline void ListAddBefore(ListHead * newItem, ListHead * item)
{
	newItem->next = item;
	newItem->prev = item->prev;
	item->prev->next = newItem;
	item->prev = newItem;
}

/**
 * Adds an item after another in the list
 *
 * @a newItem must be initialized
 *
 * @param newItem item to add after @a item
 * @param item item to insert @a newItem after
 */
static inline void ListAddAfter(ListHead * newItem, ListHead * item)
{
	newItem->prev = item;
	newItem->next = item->next;
	item->next->prev = newItem;
	item->next = newItem;
}

/**
 * Adds an item as the first item in a list
 *
 * @a newItem must be initialized
 *
 * @param newItem item to add
 * @param head head of the list to add to
 */
#define ListHeadAddFirst(newItem, head) ListAddAfter((newItem), (head))

/**
 * Adds an item as the last item in a list
 *
 * @a newItem must be initialized
 *
 * @param newItem item to add
 * @param head head of the list to add to
 */
#define ListHeadAddLast(newItem, head) ListAddBefore((newItem), (head))

/**
 * Deletes an item from a list
 *
 * The list must be re-initialized if being used again (@see ListDeleteInit)
 *
 * @param item item to remove
 */
static inline void ListDelete(ListHead * item)
{
	item->prev->next = item->next;
	item->next->prev = item->prev;
}

/**
 * Deletes an item from a list and re-initializes it
 *
 * @param item item to remove
 */
static inline void ListDeleteInit(ListHead * item)
{
	ListDelete(item);
	ListHeadInit(item);
}

/** @} */

/**
 * Converts a list head pointer into a pointer to the containing structure
 *
 * Example:
 * @code
 * struct TestList
 * {
 *     ...
 *     ListHead testListHead;
 *     ...
 * };
 *
 * ListHead * someItem = SomeFunction();
 * TestList * test = ListEntry(someItem, TestList, testListHead);
 * @endcode
 *
 * @param item list head to convert
 * @param type type of containing structure (without *)
 * @param member member of @a type which refers to the list head
 * @return pointer to the containing structure
 */
#define ListEntry(item, type, member) \
	((type *) ((char *) (item) - offsetof(type, member)))

/**
 * @name List iterators
 * @{
 */

/**
 * Iterates over a list using ::ListHead variables
 *
 * @param var loop counter (must be of type ::ListHead *)
 * @param head list to search
 */
#define ListForEach(var, head) \
	for(var = (head).next; var != (head); var = var->next)

/**
 * Iterates over a list using ::ListHead variables "safely"
 *
 * "safe" means that the current item can be removed from the list while iterating
 *
 * @param var loop counter (must be of type ::ListHead *)
 * @param tmpVar temporary variable (must be of type ::ListHead *)
 * @param head list to search
 */
#define ListForEachSafe(var, tmpVar, head) \
	for(var = (head).next, tmpVar = var->next; \
		var != (head); \
		var = tmpVar, tmpVar = tmpVar->next)

/**
 * Iterates over a list
 *
 * @param var loop counter (the type of the structure is inferred from this)
 * @param head list to search
 * @param member member in the entry structure referring to the list head
 */
#define ListForEachEntry(var, head, member) \
	for(var = ListEntry((head)->next, typeof(*(var)), member); \
		&var->member != (head); \
		var = ListEntry(var->member.next, typeof(*(var)), member))

/**
 * Iterates over a list "safely"
 *
 * "safe" means that the current item can be removed from the list while iterating
 *
 * @param var loop counter (the type of the structure is inferred from this)
 * @param tmpVar temporary variable (must same type as @a var)
 * @param head list to search
 * @param member member in the entry structure referring to the list head
 */
#define ListForEachEntrySafe(var, tmpVar, head, member) \
	for(var = ListEntry((head)->next, typeof(*(var)), member), \
		 tmpVar = ListEntry(var->member.next, typeof(*(var)), member); \
		&var->member != (head); \
		var = tmpVar, \
		 tmpVar = ListEntry(tmpVar->member.next, typeof(*(var)), member))

/**
 * Iterates over a list using ::ListHead variables in reverse
 *
 * @param var loop counter (must be of type ::ListHead *)
 * @param head list to search
 */
#define ListForEachReverse(var, head) \
	for(var = (head).prev; var != (head); var = var->prev)

/**
 * Iterates over a list using ::ListHead variables in reverse "safely"
 *
 * "safe" means that the current item can be removed from the list while iterating
 *
 * @param var loop counter (must be of type ::ListHead *)
 * @param tmpVar temporary variable (must be of type ::ListHead *)
 * @param head list to search
 */
#define ListForEachSafeReverse(var, tmpVar, head) \
	for(var = (head).prev, tmpVar = var->prev; \
		var != (head); \
		var = tmpVar, tmpVar = tmpVar->prev)

/**
 * Iterates over a list in reverse
 *
 * @param var loop counter (the type of the structure is inferred from this)
 * @param head list to search
 * @param member member in the entry structure referring to the list head
 */
#define ListForEachEntryReverse(var, head, member) \
	for(var = ListEntry((head)->prev, typeof(*(var)), member); \
		&var->member != (head); \
		var = ListEntry(var->member.prev, typeof(*(var)), member))

/**
 * Iterates over a list in reverse "safely"
 *
 * "safe" means that the current item can be removed from the list while iterating
 *
 * @param var loop counter (the type of the structure is inferred from this)
 * @param tmpVar temporary variable (must same type as @a var)
 * @param head list to search
 * @param member member in the entry structure referring to the list head
 */
#define ListForEachEntrySafeReverse(var, tmpVar, head, member) \
	for(var = ListEntry((head)->prev, typeof(*(var)), member), \
		 tmpVar = ListEntry(var->member.prev, typeof(*(var)), member); \
		&var->member != (head); \
		var = tmpVar, \
		 tmpVar = ListEntry(tmpVar->member.prev, typeof(*(var)), member))

/** @} */

#endif
