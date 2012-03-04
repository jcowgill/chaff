/*
 * htable.c
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
 *  Created on: 19 Dec 2010
 *      Author: James
 */

#include "chaff.h"
#include "htable.h"
#include "mm/kmemory.h"

//Generic Variable-Sized Hash Table
//

//Resizes an existing hash table
static void HashTableResize(HashTable * table, unsigned int newSize)
{
	//Ignore size 0
	if(newSize == 0)
	{
		return;
	}

	//Allocate new bucket table and wipe it
	HashItem ** buckets = MemVirtualZAlloc(sizeof(HashItem *) * newSize);

	//Re-add all items to buckets
	for(unsigned int i = 0; i < table->bucketCount; i++)
	{
		HashItem * currItem = table->buckets[i];
		HashItem * nextItem;

		while(currItem)
		{
			//Get next item (temp)
			nextItem = currItem->next;

			//Store in new bucket
			unsigned int bucketID = currItem->hashValue % newSize;
			currItem->next = buckets[bucketID];
			buckets[bucketID] = currItem;

			//Advance pointer
			currItem = nextItem;
		}
	}

	//Replace old bucket pointer
	if(table->buckets)
	{
		MemVirtualFree(table->buckets);
	}

	table->buckets = buckets;
	table->bucketCount = newSize;
}

//Compares 2 keys to see if their equal
static inline bool HashTableKeyCompare(const void * keyPtr1, const void * keyPtr2,
		unsigned int keyLen1, unsigned int keyLen2)
{
	//Do memory comparison
	return keyLen1 == keyLen2 && MemCmp(keyPtr1, keyPtr2, keyLen1) == 0;
}

//Checks if the hash table has reached the growing threshold
static inline bool HashTableGrowCheck(HashTable * table, unsigned int count)
{
	return count > ((table->bucketCount * HASH_THRESHOLD_GROW) / HASH_THRESHOLD_REF);
}

//Finds the hash item from a given key in a pre-calculated bucket
static inline HashItem * HashTableFindFromBucket(HashItem * bucket, const void * keyPtr, unsigned int keyLen)
{
	//Search for item
	HashItem * currItem = bucket;

	while(currItem)
	{
		//Is this the item we want?
		if(HashTableKeyCompare(currItem->keyPtr, keyPtr, currItem->keyLen, keyLen))
		{
			//This one
			return currItem;
		}

		//Advance pointer
		currItem = currItem->next;
	}

	//Not found
	return NULL;
}

//Inserts an item into the hash map
bool HashTableInsert(HashTable * table, HashItem * item, const void * keyPtr, unsigned int keyLen)
{
	//Calculate hash and store in item
	item->keyPtr = keyPtr;
	item->keyLen = keyLen;
	item->hashValue = HashTableHash(keyPtr, keyLen);

	//Ensure table is large enough
	if(HashTableGrowCheck(table, table->itemCount + 1))
	{
		//Resize
		if(table->bucketCount == 0)
		{
			HashTableResize(table, HASH_INITIAL_SIZE);
		}
		else
		{
			HashTableResize(table, table->bucketCount * 2);
		}
	}

	//Lookup bucket
	unsigned int bucketID = item->hashValue % table->bucketCount;
	HashItem * bucket = table->buckets[bucketID];

	//Check if this item is in the table
	if(HashTableFindFromBucket(bucket, keyPtr, keyLen) != NULL)
	{
		return false;
	}

	//Insert at beginning of this bucket
	item->next = bucket;
	table->buckets[bucketID] = item;
	table->itemCount++;

	return true;
}

//Removes an entry in the hash table with the given key or which is the given item
static bool HashTableRemoveKeyItem(HashTable * table, HashItem * item,
		const void * keyPtr, unsigned int keyLen)
{
	//Ignore if count == 0
	if(table->itemCount == 0)
	{
		return false;
	}

	//Calculate hash
	unsigned int bucketID;
	if(item == NULL)
	{
		bucketID = HashTableHash(keyPtr, keyLen) % table->bucketCount;
	}
	else
	{
		bucketID = item->hashValue % table->bucketCount;
	}

	//Search for item
	HashItem ** currItemPtr = &table->buckets[bucketID];
	HashItem * currItem = *currItemPtr;

	while(currItem)
	{
		//Is this the item we want?
		if(currItem == item || (item == NULL &&
				HashTableKeyCompare(currItem->keyPtr, keyPtr, currItem->keyLen, keyLen)))
		{
			//Adjust previous pointer
			*currItemPtr = currItem->next;
			currItem->next = NULL;

			//Decrement number of items
			table->itemCount--;
			return true;
		}

		//Advance pointer
		currItemPtr = &currItem->next;
		currItem = *currItemPtr;
	}

	//Not found
	return false;
}

//Removes the given ID from the hash table (provide 1 key after table)
bool HashTableRemove(HashTable * table, const void * keyPtr, unsigned int keyLen)
{
	return HashTableRemoveKeyItem(table, NULL, keyPtr, keyLen);
}

//Removes the given item from the hash table
bool HashTableRemoveItem(HashTable * table, HashItem * item)
{
	return HashTableRemoveKeyItem(table, item, item->keyPtr, item->keyLen);
}

//Returns the HashItem corresponding to a given ID (provide 1 key after table)
HashItem * HashTableFind(HashTable * table, const void * keyPtr, unsigned int keyLen)
{
	//Ignore if count == 0
	if(table->itemCount == 0)
	{
		return NULL;
	}

	//Calculate hash and get bucket
	unsigned int bucketID = HashTableHash(keyPtr, keyLen) % table->bucketCount;
	HashItem * bucket = table->buckets[bucketID];

	//Find from bucket
	return HashTableFindFromBucket(bucket, keyPtr, keyLen);
}

//Causes the hash table to grow if it will reach the grow threshold when
// storing the given number of items
void HashTableReserve(HashTable * table, unsigned int count)
{
	//Force count to be less than 1 billion
	if(count > 0x40000000)
	{
		count = 0x40000000;
	}

	//Resize if too small
	if(HashTableGrowCheck(table, count))
	{
		//Double size until large enough
		unsigned int currSize = table->bucketCount;
		while(currSize < count)
		{
			currSize *= 2;
		}

		//Resize
		HashTableResize(table, currSize);
	}
}

//Shrinks the hash table if there are very few items in it
void HashTableShrink(HashTable * table)
{
	//Check shrink threshold
	unsigned int shrinkThreshold = (table->bucketCount * HASH_THRESHOLD_SHRINK) / HASH_THRESHOLD_REF;
	if(table->bucketCount < shrinkThreshold)
	{
		//Divide size until small enough
		unsigned int currSize = table->bucketCount;
		while(currSize > shrinkThreshold)
		{
			currSize /= 2;
		}

		//Resize table
		if(currSize != 0)
		{
			HashTableResize(table, currSize);
		}
	}
}

//Hashes the key using the built-in hashing function
unsigned int HashTableHash(const void * keyPtr, unsigned int keyLen)
{
	//FNV Hash Algorithm
	const char * mem = keyPtr;
	unsigned int hash = 2166136261;

	while(keyLen-- > 0)
	{
		hash ^= *mem++;
		hash *= 16777619;
	}

	return hash;
}
