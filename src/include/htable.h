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

#include "list.h"

//Simple 256 bucket hash table
#define HASHT_BUCKET_COUNT 256

typedef struct tagSHashItem
{
	unsigned int id;
	struct tagSHashItem * next;

} HashItem;

typedef HashItem * HashTable[HASHT_BUCKET_COUNT];

//Insert an item into the hashmap
// You must set the ID in the HashItem
// Returns false if that ID already exists
bool HashTableInsert(HashTable table, HashItem * item);

//Removes the given ID from the hash table
// Returns false if that ID doesn't exist
bool HashTableRemove(HashTable table, unsigned int id);

//Returns the HashItem corresponding to a given ID
// Returns NULL if that ID doesn't exist
HashItem * HashTableFind(HashTable table, unsigned int id);

//Gets the hash tale item corresponding to a given HashItem
#define HashTableEntry ListEntry

#endif /* HTABLE_H_ */
