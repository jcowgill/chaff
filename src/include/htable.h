/*
 * htable.h
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

#ifndef HTABLE_H_
#define HTABLE_H_

#include "chaff.h"
#include "list.h"

//Generic Variable-Sized Hash Table
//

//Hash table thresholds (these affect all hash tables)
#define HASH_INITIAL_SIZE		256

//Thresholds work like fractions of (threshold / HASH_THRESHOLD_REF)
#define HASH_THRESHOLD_REF		8
#define HASH_THRESHOLD_GROW		7		// 7/8 = 87.5% load
#define HASH_THRESHOLD_SHRINK	1		// 1/8 = 12.5% load

//Hash Item
// Add this to all items in the hash table
typedef struct HashItem
{
	//Pointer and length of key
	const void * keyPtr;
	unsigned int keyLen;

	//Hash of this key
	unsigned int hashValue;

	//Next item in this bucket
	struct HashItem * next;

} HashItem;

//Hash Table
// This structure contains the buckets at the root of the table
// This must be initialized to 0
typedef struct HashTable
{
	//Pointer to array of buckets and number of items in array
	HashItem ** buckets;
	unsigned int bucketCount;

	//Numer of items in the hash table
	unsigned int itemCount;

} HashTable;

//Gets the structure associated with a hash item
#define HashTableEntry ListEntry

//Inserts an item into the hash map
// The key passed must remain in memory after this returns
bool HashTableInsert(HashTable * table, HashItem * item, const void * keyPtr, unsigned int keyLen);

//Removes the given ID from the hash table (provide 1 key after table)
// Returns false if that ID doesn't exist
bool HashTableRemove(HashTable * table, const void * keyPtr, unsigned int keyLen);

//Removes the given item from the hash table
// Returns false if that ID doesn't exist
bool HashTableRemoveItem(HashTable * table, HashItem * item);

//Returns the HashItem corresponding to a given ID (provide 1 key after table)
// Returns NULL if that ID doesn't exist
HashItem * HashTableFind(HashTable * table, const void * keyPtr, unsigned int keyLen);

//Causes the hash table to grow if it will reach the grow threshold when
//storing the given number of items
// Does not gaurentee that a later HashTableInsert will not grow the table
void HashTableReserve(HashTable * table, unsigned int count);

//Shrinks the hash table if there are very few items in it
void HashTableShrink(HashTable * table);

//Returns the number of items in the hash table
static inline unsigned int HashTableCount(HashTable * table)
{
	return table->itemCount;
}

//Hashes the key using the built-in hashing function
unsigned int HashTableHash(const void * keyPtr, unsigned int keyLen);

#endif /* HTABLE_H_ */
