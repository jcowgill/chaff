/*
 * htable.c
 *
 *  Created on: 19 Dec 2010
 *      Author: James
 */

#include "chaff.h"
#include "htable.h"

//Simple 256 bucket hash table

static inline unsigned int HashFunction(unsigned int id)
{
	//Simplest function
	return id % HASHT_BUCKET_COUNT;
}

//Insert an item into the hashmap
// You must set the ID in the HashItem
// Returns false if that ID already exists
bool HashTableInsertItem(HashTable table, HashItem * item)
{
	//Get bucket for hash
	HashItem ** ptrToItem = &table[HashFunction(item->id)];
	HashItem * currItem = *ptrToItem;

	//Find id
	while(currItem != NULL)
	{
		if(currItem->id > item->id)
		{
			//Insert before this item
			item->next = currItem;
			*ptrToItem = item;
			return true;
		}
		else if(currItem->id == item->id)
		{
			//Already in hash table
			return false;
		}

		//Next item
		ptrToItem = &currItem->next;
		currItem = *ptrToItem;
	}

	//Insert at end
	item->next = NULL;
	*ptrToItem = item;
	return true;
}

//Removes the given ID from the hash table
// Returns false if that ID doesn't exist
bool HashTableRemove(HashTable table, unsigned int id)
{
	//Get bucket for hash
	HashItem ** ptrToItem = &table[HashFunction(id)];
	HashItem * currItem = *ptrToItem;

	//Find id
	while(currItem != NULL)
	{
		if(currItem->id == id)
		{
			//Change pointer to item to this item's next value
			*ptrToItem = currItem->next;
			currItem->next = NULL;
			return true;
		}
		else if(currItem->id > id)
		{
			//Not found
			return false;
		}

		//Next item
		ptrToItem = &currItem->next;
		currItem = *ptrToItem;
	}

	//Not found
	return false;
}

//Returns the HashItem corresponding to a given ID
// Returns NULL if that ID doesn't exist
HashItem * HashTableFind(HashTable table, unsigned int id)
{
	//Get first item in bucket
	HashItem * item = table[HashFunction(id)];

	//Find the item, starting with the first bucket
	while(item != NULL)
	{
		//Check ID
		if(item->id == id)
		{
			//Found item
			return item;
		}

		//Next item
		item = item->next;
	}

	//Item not found
	return NULL;
}
