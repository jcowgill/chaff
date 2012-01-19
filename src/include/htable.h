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

//Simple 256 bucket hash table
#define HASHT_BUCKET_COUNT 256

typedef struct tagSHashItem
{
	struct tagSHashItem * next;

} HashItem;

//Hash table data
typedef struct
{
	const void * (* key)(HashItem * item);		//Gets a key from an item
	unsigned int (* hash)(const void * key);
	bool (* compare)(const void * key1, const void * key2);

	HashItem * table[HASHT_BUCKET_COUNT];
	int count;

} HashTable;

//String hash table which will also handle strings specified by length
// instead of being null terminated
typedef struct
{
	//Gets the key (and possibly length) for a string hash item
	// Returns true if length is used. Otherwise str must be null terminated.
	bool (* key)(HashItem * item, const char ** str, int * len);

	HashItem * table[HASHT_BUCKET_COUNT];
	int count;

} HashTableString;

//Insert an item into the hashmap
// You must set the ID in the HashItem
// Returns false if that ID already exists
bool HashTableInsert(HashTable * table, HashItem * item);

//Removes the given ID from the hash table (provide 1 key after table)
// Returns false if that ID doesn't exist
bool HashTableRemove(HashTable * table, ...);

//Removes the given item from the hash table
// Returns false if that ID doesn't exist
bool HashTableRemoveItem(HashTable * table, HashItem * item);

//Returns the HashItem corresponding to a given ID (provide 1 key after table)
// Returns NULL if that ID doesn't exist
HashItem * HashTableFind(HashTable * table, ...);


//Insert an item into the string hashmap
// You must set the ID in the HashItem
// Returns false if that ID already exists
bool HashTableStringInsert(HashTableString * table, HashItem * item);

//Generic remove - dont use this, use one of the other removers
bool HashTableStringRemoveGeneric(HashTableString * table, bool usingLen, const char * str, int len);

//Removes the given item from the string hash table (using null terminated string)
// Returns false if that ID doesn't exist
static inline bool HashTableStringRemove(HashTableString * table, const char * str)
{
	return HashTableStringRemoveGeneric(table, false, str, 0);
}

//Removes the given item from the string hash table (using string length)
// Returns false if that ID doesn't exist
static inline bool HashTableStringRemoveLen(HashTableString * table, const char * str, int len)
{
	return HashTableStringRemoveGeneric(table, true, str, len);
}

//Removes the given item from the hash table
// Returns false if that ID doesn't exist
bool HashTableStringRemoveItem(HashTableString * table, HashItem * item);

//Generic find - don't use this, use one of the other removers
HashItem * HashTableStringFindGeneric(HashTableString * table, bool usingLen, const char * str, int len);

//Returns the HashItem corresponding to a given string (using null terminated string)
// Returns NULL if that ID doesn't exist
static inline HashItem * HashTableStringFind(HashTableString * table, const char * str)
{
	return HashTableStringFindGeneric(table, false, str, 0);
}

//Returns the HashItem corresponding to a given string (using string length)
// Returns NULL if that ID doesn't exist
static inline HashItem * HashTableStringFindLen(HashTableString * table, const char * str, int len)
{
	return HashTableStringFindGeneric(table, false, str, len);
}


//Gets the hash tale item corresponding to a given HashItem
#define HashTableEntry ListEntry

//Helper hash functions
unsigned int HashTableIntHash(const void * num);
unsigned int HashTableMemHash(const void * mem, unsigned int size);

static inline unsigned int HashTableIntHashHelp(unsigned int num)
{
	return HashTableMemHash(&num, sizeof(unsigned int));
}

//Compares the pointers given - use for ints and ptrs
bool HashTableCompare(const void * num1, const void * num2);

#endif /* HTABLE_H_ */
