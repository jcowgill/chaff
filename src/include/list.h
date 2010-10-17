/*
 * list.h
 *
 *  Created on: 16 Oct 2010
 *      Author: James
 */

#ifndef LIST_H_
#define LIST_H_

//The linked list classes
// These are based from the design at http://locklessinc.com/articles/flexible_lists_in_cpp/

namespace Chaff
{
	//A single entry in a list
	// Use DECLARE_LISTENTRY instead of instantiating this class
	class ListEntry
	{
	public:
		ListEntry * next;
		ListEntry * prev;

		//Creates a new list entry with this as the only item in the list
		ListEntry()
			:next(this), prev(this)
		{
		}

		//Creates a new list entry which is inserted after the given entry in the list
		ListEntry(ListEntry& entry)
		{
			InsertAfter(entry);
		}

		//Destroys this list entry (automatically detaches it)
		~ListEntry()
		{
			next->prev = prev;
			prev->next = next;
		}

		//Inserts this entry after another entry
		// Before inserting this entry into the list again, it must be detached
		void InsertAfter(ListEntry& entry)
		{
			prev = &entry;
			next = entry.next;
			entry.next->prev = this;
			entry.next = this;
		}

		//Inserts this entry after another entry
		// Before inserting this entry into the list again, it must be detached
		void InsertBefore(ListEntry& entry)
		{
			next = &entry;
			prev = entry.prev;
			entry.prev->next = this;
			entry.prev = this;
		}

		//Detaches this list entry from the list it is in and puts it in its own list
		void Detach()
		{
			next->prev = prev;
			prev->next = next;
			next = this;
			prev = this;
		}
	};

	//An easy to use template which can be used to iterator over a list
	// just like you would with the standard c++ containers
	template <class T, unsigned int offset(void)> class ListIterator
	{
	private:
		ListEntry * entry;

		//Returns the object that this iterator is currently pointing to
		T * Container() const
		{
			return reinterpret_cast<T *>(((unsigned int) entry) - offset());
		}

	public:
		//Constructors
		ListIterator(T * item)
			:ListIterator(*item)
		{
		}

		ListIterator(T& item)
			:entry(reinterpret_cast<ListEntry *>(((unsigned int) item) + offset()))
		{
		}

		ListIterator(ListEntry * entry)
			:entry(entry)
		{
		}

		ListIterator(ListEntry& entry)
			:entry(&entry)
		{
		}

		ListIterator(const ListIterator& iter)
			:entry(iter.entry)
		{
		}

		//Assignment operator
		ListIterator& operator= (const ListIterator& iter)
		{
			entry = iter.entry;
			return *this;
		}

		ListIterator& operator= (ListEntry& entry)
		{
			this->entry = &entry;
			return *this;
		}

		ListIterator& operator= (ListEntry * entry)
		{
			this->entry = entry;
			return *this;
		}

		//Container getters
		const T& operator* () const
		{
			return *Container();
		}

		const T * operator-> () const
		{
			return Container();
		}

		T& operator* ()
		{
			return *Container();
		}

		T * operator-> ()
		{
			return Container();
		}

		//Moves the iterator to point to the next item in the list
		ListIterator& operator++()
		{
			entry = entry->next;
			return *this;
		}

		//Moves the iterator to point to the previous item in the list
		ListIterator& operator--()
		{
			entry = entry->prev;
			return *this;
		}

		//Moves the iterator to point to the next item in the list (post increment)
		ListIterator operator++ (int)
		{
			ListIterator prevIter = *this;
			entry = entry->next;
			return prevIter;
		}

		//Moves the iterator to point to the previous item in the list (post increment)
		ListIterator operator-- (int)
		{
			ListIterator prevIter = *this;
			entry = entry->prev;
			return prevIter;
		}

		//Comparisons
		bool operator== (const ListIterator& iter) const
		{
			return entry == iter.enntry;
		}

		bool operator!= (const ListIterator& iter) const
		{
			return entry != iter.enntry;
		}
	};

	//A helper class to store the first entry in a list
	// This class adds a dummy entry to the list to act as the "head"
	// This entry should NOT be dereferenced with the ListIterator class.
	// This class does not free its children when it is destructed
	template <class T, unsigned int offset(void)> class ListHead
	{
	private:
		//List dummy entry
		ListEntry headEntry;

	public:
		typedef ListIterator<T, offset> Iterator;

		//Returns an iterator pointing to the first item or End if the list is empty
		Iterator Begin()
		{
			return Iterator(headEntry.next);
		}

		//Returns an iterator pointing to the last item or End if the list is empty
		Iterator Last()
		{
			return Iterator(headEntry.prev);
		}

		//Returns an iterator pointing to the dummy end item (between last and first)
		Iterator End()
		{
			return Iterator(headEntry);
		}

		//Inserts an item at the beginning of the list
		void InsertFirst(T& item)
		{
			headEntry.InsertAfter(*reinterpret_cast<ListEntry *>((unsigned int) &item + offset()));
		}

		//Inserts an item at the beginning of the list
		void InsertLast(T& item)
		{
			headEntry.InsertBefore(*reinterpret_cast<ListEntry *>((unsigned int) &item + offset()));
		}

		//Empty checker
		bool Empty() const
		{
			return headEntry.next == &headEntry;
		}

		//Clears the list (but doesn't deallocate any objects)
		void Clear()
		{
			headEntry.Detach();
		}
	};

	//Defines for automatically handling offset template function
#define offsetof_hack(C, M) (reinterpret_cast<unsigned int>(\
		&reinterpret_cast<char &>(reinterpret_cast<C *> (1)->M))\
	 - 1)

#define DECLARE_LISTENTRY(C, M)\
	static unsigned int offset_##M() {return offsetof_hack(C, M);}\
	ListEntry M

#define DECLARE_LISTITERATOR(C, M)\
	ListIterator<C, C::offset_##M>

#define DECLARE_LISTHEAD(C, M)\
	ListHead<C, C::offset_##M>

}

#endif /* LIST_H_ */
