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

//Gets the hash tale item corresponding to a given HashItem
#define HashTableEntry ListEntry

//Helper hash functions
unsigned int HashTableIntHash(const void * num);
unsigned int HashTableStrHash(const void * str);

unsigned int HashTableMemHash(const void * mem, unsigned int size);

static inline unsigned int HashTableIntHashHelp(unsigned int num)
{
	return HashTableMemHash(&num, sizeof(unsigned int));
}

static inline unsigned int HashTableStrHashHelp(const char * str)
{
	return HashTableMemHash(str, StrLen(str));
}

bool HashTableCompare(const void * num1, const void * num2);		//Compares the pointers given - use for ints and ptrs
bool HashTableStrCompare(const void * str1, const void * str2);		//Compares the strings given

#endif /* HTABLE_H_ */
