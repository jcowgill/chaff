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

//Simple 256 bucket hash table

static bool HashTableStringMixedCompare(HashTableString * table, bool usingLen,
		const char * str, int len, HashItem * other);
static unsigned int HashTableStringHash(bool usingLen, const char * str, int len);

//Insert an item into the hashmap
// You must set the ID in the HashItem
// Returns false if that ID already exists
bool HashTableInsert(HashTable * table, HashItem * item)
{
	//Get bucket for hash
	const void * key = table->key(item);
	unsigned int hash = table->hash(key) % HASHT_BUCKET_COUNT;

	HashItem * currItem = table->table[hash];

	//Setup new item
	item->next = currItem;

	//Check if in table
	while(currItem != NULL)
	{
		if(table->compare(key, table->key(currItem)))
		{
			//Already in hash table
			return false;
		}

		currItem = currItem->next;
	}

	//Insert at beginning
	table->table[hash] = item;
	table->count++;
	return true;
}

//Removes the given ID from the hash table (provide 1 key after table)
// Returns false if that ID doesn't exist
bool HashTableRemove(HashTable * table, ...)
{
	//Get key
	const void * key;

	va_list args;
	va_start(args, table);
	key = va_arg(args, const void *);
	va_end(args);

	//Get bucket for hash
	unsigned int hash = table->hash(key) % HASHT_BUCKET_COUNT;

	HashItem ** ptrToItem = &table->table[hash];
	HashItem * currItem = *ptrToItem;

	//Find item
	while(currItem != NULL)
	{
		if(table->compare(key, table->key(currItem)))
		{
			//Change pointer to item to this item's next value
			*ptrToItem = currItem->next;
			currItem->next = NULL;
			table->count--;
			return true;
		}

		//Next item
		ptrToItem = &currItem->next;
		currItem = *ptrToItem;
	}

	//Not found
	return false;
}

//Removes the given item from the hash table
// Returns false if that ID doesn't exist
bool HashTableRemoveItem(HashTable * table, HashItem * item)
{
#warning must only remove if the item itself is the same
	return HashTableRemove(table, table->key(item));
}

//Returns the HashItem corresponding to a given ID (provide 1 key after table)
// Returns NULL if that ID doesn't exist
HashItem * HashTableFind(HashTable * table, ...)
{
	//Get key
	const void * key;

	va_list args;
	va_start(args, table);
	key = va_arg(args, const void *);
	va_end(args);

	//Get bucket for hash
	unsigned int hash = table->hash(key) % HASHT_BUCKET_COUNT;

	HashItem * item = table->table[hash];

	//Find the item, starting with the first bucket
	while(item != NULL)
	{
		//Check ID
		if(table->compare(key, table->key(item)))
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

//Insert an item into the string hashmap
// You must set the ID in the HashItem
// Returns false if that ID already exists
bool HashTableStringInsert(HashTableString * table, HashItem * item)
{
	//Get key
	const char * str;
	int len;
	bool usingLen = table->key(item, &str, &len);

	//Get hash
	unsigned int hash = HashTableStringHash(usingLen, str, len);

	//Get bucket
	HashItem * currItem = table->table[hash];

	//Setup new item
	item->next = currItem;

	//Check if in table
	while(currItem != NULL)
	{
		if(HashTableStringMixedCompare(table, usingLen, str, len, currItem))
		{
			//Already in hash table
			return false;
		}

		currItem = currItem->next;
	}

	//Insert at beginning
	table->table[hash] = item;
	table->count++;
	return true;
}

//Removes the given item from the string hash table
// Returns false if that ID doesn't exist
bool HashTableStringRemoveGeneric(HashTableString * table, bool usingLen, const char * str, int len)
{
	//Get bucket for hash
	unsigned int hash = HashTableStringHash(usingLen, str, len);

	HashItem ** ptrToItem = &table->table[hash];
	HashItem * currItem = *ptrToItem;

	//Find item
	while(currItem != NULL)
	{
		if(HashTableStringMixedCompare(table, usingLen, str, len, currItem))
		{
			//Change pointer to item to this item's next value
			*ptrToItem = currItem->next;
			currItem->next = NULL;
			table->count--;
			return true;
		}

		//Next item
		ptrToItem = &currItem->next;
		currItem = *ptrToItem;
	}

	//Not found
	return false;
}

//Removes the given item from the hash table
// Returns false if that ID doesn't exist
bool HashTableStringRemoveItem(HashTableString * table, HashItem * item)
{
	const char * str;
	int len;
	bool usingLen = table->key(item, &str, &len);

	return HashTableStringRemoveGeneric(table, usingLen, str, len);
}

//Returns the HashItem corresponding to a given string
// Returns NULL if that ID doesn't exist
HashItem * HashTableStringFindGeneric(HashTableString * table, bool usingLen, const char * str, int len)
{
	//Get bucket for hash
	unsigned int hash = HashTableStringHash(usingLen, str, len);

	HashItem * item = table->table[hash];

	//Find the item, starting with the first bucket
	while(item != NULL)
	{
		//Chrck ID
		if(HashTableStringMixedCompare(table, usingLen, str, len, item))
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


//Hash related functions
unsigned int HashTableIntHash(const void * num)
{
	return HashTableIntHashHelp((unsigned int) num);
}

unsigned int HashTableMemHash(const void * memIn, unsigned int size)
{
	//This is the FNV hash algorithm
	const char * mem = memIn;
	unsigned int hash = 2166136261;

	while(size-- > 0)
	{
		hash ^= *mem++;
		hash *= 16777619;
	}

	return hash;
}

bool HashTableCompare(const void * num1, const void * num2)
{
	return num1 == num2;
}

//Internal string helpers

//Compares a hash item which possibly uses mixed types (null terminated and length given)
static bool HashTableStringMixedCompare(HashTableString * table, bool usingLen,
		const char * str, int len, HashItem * other)
{
	//Get other key
	const char * oStr;
	int oLen;

	if(table->key(other, &oStr, &oLen))
	{
		//Other uses len
		if(!usingLen)
		{
			len = StrLen(str);
		}
	}
	else
	{
		//Other uses zterm
		if(usingLen)
		{
			//Do a memory comparison
			oLen = StrLen(oStr);
		}
		else
		{
			//Quicker than doing 2 strlens
			return StrCmp(str, oStr) == 0;
		}
	}

	//If we're here, both are using lengths so we can do a memory compraison
	return len == oLen && MemCmp(str, oStr, len) == 0;
}

//Hashes the given string
static unsigned int HashTableStringHash(bool usingLen, const char * str, int len)
{
	if(usingLen)
	{
		return HashTableMemHash(str, len) % HASHT_BUCKET_COUNT;
	}
	else
	{
		return HashTableMemHash(str, StrLen(str)) % HASHT_BUCKET_COUNT;
	}
}
