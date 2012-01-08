/*
 * htable.h
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
