/*
 * chaff.h
 *
 *  Created on: 30 Sep 2010
 *      Author: James
 */

#ifndef LIST_H_
#define LIST_H_

//The linked list class

namespace Chaff
{
	//A linked list for use in the kernel
	// Unlike the STL list, this list works with pointers and does not own the objects
	// The destructor just destroys the list - but YOU need to destroy the items in it!
	template <class T> class List
	{
	private:
		//A list node structure
		struct Node
		{
			//Note: this must have the same size as NodeBase
			Node * next;
			Node * prev;
			T * data;

			//Constructor
			Node(Node * next, Node * prev, T * data)
				:next(next), prev(prev), data(data)
			{
			}
		};

		//First and last nodes in the list
		Node * first;
		Node * last;

		//The size of the list
		int size;

	public:
		//A class used for iterating over the items in a list
		class Iterator
		{
		private:
			friend class List;

			//Pointer to the current node
			Node * node;

			//Create iterator from node
			Iterator(Node * node)
				: node(node)
			{
			}

		public:
			//Creates a new blank iterator
			// Performing anything on this iterator will crash the kernel!
			Iterator()
				:node(NULL)
			{
			}

			//Creates a new iterator based on another
			Iterator(const Iterator& iterator)
				: node(iterator.node)
			{
			}

			//Assigns an iterator to this iterator
			Iterator& operator= (const Iterator& iterator)
			{
				node = iterator.node;
			}

			//Increments the position of the iterator
			Iterator& operator++ ()
			{
				node = node->next;
			}

			//Decrements the position of the iterator
			Iterator& operator-- ()
			{
				node = node->prev;
			}

			//Increments the position of the iterator (postfix)
			Iterator operator++ (int)
			{
				Iterator prevIter = *this;
				node = node->next;
				return prevIter;
			}

			//Decrements the position of the iterator (postfix)
			Iterator operator-- (int)
			{
				Iterator prevIter = *this;
				node = node->prev;
				return prevIter;
			}

			//Dereferences the iterator to get the data
			T *& operator* () const
			{
				return node->data;
			}

			//Dereferences the iterator to get the data
			T& operator-> () const
			{
				return *node->data;
			}

			//Returns weather this iterator refers to a NULL position
			// This happens after a default construction or being incremented
			// or decremented off the end of the list
			bool IsNull() const
			{
				return node == NULL;
			}

			//Compares this iterator with another
			bool operator== (const Iterator& iterator) const
			{
				return node == iterator.node;
			}

			//Compares this iterator with another
			bool operator!= (const Iterator& iterator) const
			{
				return node != iterator.node;
			}
		};

		//A class used for iterating over the items in a constant list
		class ConstIterator
		{
		private:
			//Pointer to the current node
			const Node * node;

			//Create iterator from node
			ConstIterator(const Node * node)
				: node(node)
			{
			}

		public:
			//Creates a new blank iterator
			// Performing anything on this iterator will crash the kernel!
			ConstIterator()
				:node(NULL)
			{
			}

			//Creates a new iterator based on another
			ConstIterator(const ConstIterator& iterator)
				: node(iterator.node)
			{
			}

			//Creates a new iterator based on another
			ConstIterator(const Iterator& iterator)
				: node(iterator.node)
			{
			}

			//Assigns an iterator to this iterator
			ConstIterator& operator= (const ConstIterator& iterator)
			{
				node = iterator.node;
				return *this;
			}

			//Assigns an iterator to this iterator
			ConstIterator& operator= (const Iterator& iterator)
			{
				node = iterator.node;
				return *this;
			}

			//Increments the position of the iterator
			ConstIterator& operator++ ()
			{
				node = node->next;
				return *this;
			}

			//Decrements the position of the iterator
			ConstIterator& operator-- ()
			{
				node = node->prev;
				return *this;
			}

			//Increments the position of the iterator (postfix)
			ConstIterator operator++ (int)
			{
				ConstIterator prevIter = *this;
				node = node->next;
				return prevIter;
			}

			//Decrements the position of the iterator (postfix)
			ConstIterator operator-- (int)
			{
				ConstIterator prevIter = *this;
				node = node->prev;
				return prevIter;
			}

			//Dereferences the iterator to get the data
			const T *& operator* () const
			{
				return node->data;
			}

			//Dereferences the iterator to get the data
			const T& operator-> () const
			{
				return *node->data;
			}

			//Returns weather this iterator refers to a NULL position
			// This happens after a default construction or being incremented
			// or decremented off the end of the list
			bool IsNull() const
			{
				return node == NULL;
			}

			//Compares this iterator with another
			bool operator== (const Iterator& iterator) const
			{
				return node == iterator.node;
			}

			//Compares this iterator with another
			bool operator!= (const Iterator& iterator) const
			{
				return node != iterator.node;
			}

			//Compares this iterator with another
			bool operator== (const ConstIterator& iterator) const
			{
				return node == iterator.node;
			}

			//Compares this iterator with another
			bool operator!= (const ConstIterator& iterator) const
			{
				return node != iterator.node;
			}
		};

		//Initialises a new linked list
		List()
			:size(0), first(NULL), last(NULL)
		{
		}

		//Destructs the list and nodes
		~List()
		{
			Clear();
		}

		//Iterator getters
		Iterator First()
		{
			return Iterator(first);
		}

		Iterator Last()
		{
			return Iterator(last);
		}

		ConstIterator First() const
		{
			return ConstIterator(first);
		}

		ConstIterator Last() const
		{
			return ConstIterator(last);
		}

		//Value getters
		T * FirstValue()
		{
			return first->data;
		}

		T * LastValue()
		{
			return last->data;
		}

		//Returns the number of items in the list
		int Size() const
		{
			return size;
		}

		//True if this list has no items
		bool Empty() const
		{
			return size == 0;
		}

		//Removes all the nodes from the list
		void Clear()
		{
			Node * currNode = first;
			Node * nextNode;

			//Free all the nodes back to the pool
			while(currNode)
			{
				nextNode = currNode->next;
				delete currNode;
				currNode = nextNode;
			}

			//Set pointers
			first = NULL;
			last = NULL;
			size = 0;
		}

		//Inserts some data after the given position
		Iterator InsertAfter(const Iterator position, T * data)
		{
			//Create a new node
			Node * newNode = new Node(position.node->next, position.node, data);

			//Update position pointers
			position.node->next->prev = newNode;
			position.node->next = newNode;

			//Check if this should be the last pos now
			if(newNode->next == NULL)
			{
				last = newNode;
			}

			//Increment size
			++size;

			//Return iterator to node
			return Iterator(newNode);
		}

		//Inserts some data before the given position
		Iterator InsertBefore(const Iterator position, T * data)
		{
			//Create a new node
			Node * newNode = new Node(position.node, position.node->prev, data);

			//Update position pointers
			position.node->prev->next = newNode;
			position.node->prev = newNode;

			//Check if this should be the first pos now
			if(newNode->prev == NULL)
			{
				first = newNode;
			}

			//Increment size
			++size;

			//Return iterator to node
			return Iterator(newNode);
		}

		//Erases the item at the given position
		void Erase(const Iterator position)
		{
			//Update surrounding nodes
			if(position.node->next == NULL)
			{
				last = position.node->prev;
			}
			else
			{
				position.node->next->prev = position.node->prev;
			}

			if(position.node->prev == NULL)
			{
				first = position.node->next;
			}
			else
			{
				position.node->prev->next = position.node->next;
			}

			//Free node
			delete position.node;
		}

		//Inserts the data at the beginning of the list
		void InsertFirst(T * data)
		{
			//If this list is blank, handle separately
			if(first)
			{
				InsertBefore(first, data);
			}
			else
			{
				first = last = new Node(NULL, NULL, data);
				size = 1;
			}
		}

		//Inserts the data at the end of the list
		void InsertLast(T * data)
		{
			//If this list is blank, handle separately
			if(last)
			{
				InsertAfter(last, data);
			}
			else
			{
				first = last = new Node(NULL, NULL, data);
				size = 1;
			}
		}

		//Erases the item at the beginning of the list
		void EraseFirst()
		{
			Erase(first);
		}

		//Erases the item at the end of the list
		void EraseLast()
		{
			Erase(last);
		}
	};
}

#endif
