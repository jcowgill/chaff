/**
 * @file
 * Hash table implementation
 *
 * This file contains a generic, variable-sized hash table implementation.
 *
 * To use it:
 * - Create a ::HashTable somewhere and wipe it
 * - Add ::HashItem to each structure you want to add to the table
 * - Use the manipulation functions to use the hash table
 * - Use #HashTableEntry to convert HashItems into actual structures
 *
 * @date February 2012
 * @author James Cowgill
 * @ingroup Util
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

#ifndef HTABLE_H_
#define HTABLE_H_

#include "chaff.h"
#include "list.h"

/**
 * @name Global thresholds and performance adjusters
 */
/** @{ */

/**
 * Initial number of buckets in the table
 */
#define HASH_INITIAL_SIZE		256

/**
 * Reference for GROW and SHRINK
 */
#define HASH_THRESHOLD_REF		8

/**
 * Threshold required to grow the hash table
 *
 * 7/8 = 87.5% load
 */
#define HASH_THRESHOLD_GROW		7

/**
 * Threshold required to shrink the hash table
 *
 * 1/8 = 12.5% load
 */
#define HASH_THRESHOLD_SHRINK	1

/** @} */

/**
 * Structure storing data about an item in the hash table
 */
typedef struct HashItem
{
	/**
	 * Pointer to key used by this item
	 */
	const void * keyPtr;

	/**
	 * Length of key in bytes
	 */
	unsigned int keyLen;

	/**
	 * Cached hash value of the key
	 */
	unsigned int hashValue;

	/**
	 * Pointer to next item in the bucket
	 */
	struct HashItem * next;

} HashItem;

/**
 * Structure storing data about the entire hash table including buckets
 *
 * This must be cleared to all zeros using MemSet() before use
 */
typedef struct HashTable
{
	/**
	 * Contains all the buckets used by the hash table
	 */
	HashItem ** buckets;

	/**
	 * The number of buckets allocated for #buckets
	 */
	unsigned int bucketCount;

	/**
	 * The number of items in the table
	 */
	unsigned int itemCount;

} HashTable;

/**
 * Returns a pointer to the structure associated with a HashItem
 *
 * @see ListEntry
 */
#define HashTableEntry ListEntry

/**
 * Inserts an item into the hash map
 *
 * The key passed must remain in memory after this returns
 *
 * @param table hash table to insert into
 * @param item item to add to the table
 * @param keyPtr pointer to the key for this item
 * @param keyLen length of the key in bytes
 *
 * @retval true if the item was successfully added
 * @retval false if the item already exists
 */
bool HashTableInsert(HashTable * table, HashItem * item, const void * keyPtr, unsigned int keyLen);

/**
 * Removes a key from the hash map
 *
 * @param table hash table to remove from
 * @param keyPtr pointer to the key to remove
 * @param keyLen length of the key in bytes
 *
 * @retval true if the item was successfully removed
 * @retval false if an item with that key does not exists
 */
bool HashTableRemove(HashTable * table, const void * keyPtr, unsigned int keyLen);

/**
 * Removes an item from the hash map
 *
 * @param table hash table to remove from
 * @param item the item to remove
 *
 * @retval true if the item was successfully removed
 * @retval false if that item is not in the hash table
 */
bool HashTableRemoveItem(HashTable * table, HashItem * item);

/**
 * Finds an item in the hash table
 *
 * @param table hash table to find in
 * @param keyPtr pointer to the key to find
 * @param keyLen length of the key in bytes
 *
 * @return the hash item requested or NULL if that item is not in the hash table
 */
HashItem * HashTableFind(HashTable * table, const void * keyPtr, unsigned int keyLen);

/**
 * Grows the hash table.
 *
 * This function grows the hash table if it will reach the grow threshold when storing
 * the given number of items.
 *
 * Does not guarantee that a later HashTableInsert will not grow the table
 *
 * @param table hash table to grow
 * @param count the @a total number of items expected to be in the hash table
 */
void HashTableReserve(HashTable * table, unsigned int count);

/**
 * Shrinks the hash table if there are very few items in it
 *
 * @param table hash table to shrink
 */
void HashTableShrink(HashTable * table);

/**
 * Returns the number of items in the hash table
 */
static inline unsigned int HashTableCount(HashTable * table)
{
	return table->itemCount;
}

/**
 * Hashes the given key using the built-in hashing function
 *
 * @param keyPtr pointer to the key to hash
 * @param keyLen length of the key in bytes
 * @return the computed hash of the key
 */
unsigned int HashTableHash(const void * keyPtr, unsigned int keyLen);

#endif /* HTABLE_H_ */
