/*
 * pool.h
 *
 *  Created on: 3 Oct 2010
 *      Author: James
 */

#ifndef POOL_H_
#define POOL_H_

namespace Chaff
{
	//An abstract base class for a generic kernel memory pool
	// which supplies memory for a particular object
	//
	// sizeof(T) MUST be at least 4 bytes!
	template <class T> class MemPool
	{
	private:
		//A block of memory assigned to the memory pool
		// These should not be created directly
		struct MemBlock
		{
			MemBlock * nextBlock;
			unsigned int dummyMemStart;
		};

		//Used to chain pieces of free memory together
		struct MemChain
		{
			MemChain * next;
		};

		//The memory block size in dataSizes
		unsigned int blockSize;

		//The head memory block (the youngest one)
		MemBlock * headBlock;

		//The number of Ts allocated from the memory block
		unsigned int allocatedFromBlock;

		//The head memory entry in the free memory chain
		MemChain * headChain;


	public:
		//Creates a new memory pool
		// The blockSize is the number of Ts to allocate per block
		MemPool(unsigned int blockSize = 128)
			: blockSize(blockSize), allocatedFromBlock(0), headChain(NULL)
		{
			//Create the first memory block
			headBlock = reinterpret_cast<MemBlock *>(new char[sizeof(MemBlock *) + (blockSize * sizeof(T))]);

			//Set no next block
			headBlock->nextBlock = NULL;
		}

		//Destructs the memory pool
		// All memory allocated using the pool will be destroyed - even if it has not be freed
		~MemPool()
		{
			//Free all the memory blocks
			MemBlock * currBlock;
			MemBlock * nextBlock = headBlock;

			//Process block
			do
			{
				//Get next block
				currBlock = nextBlock;
				nextBlock = nextBlock->nextBlock;

				//Free the block
				delete [] reinterpret_cast<char *>(currBlock);

			} while(nextBlock);
		}

		//Allocates 1 object and calls its constructor
		T * Allocate()
		{
			//Use free chain if possible
			if(headChain)
			{
				MemChain * toReturn = headChain;

				//Move on chain
				headChain = toReturn->next;

				//Create and return object
				return new(reinterpret_cast<T *>(toReturn)) T();
			}
			else
			{
				//Any memory left in block?
				if(allocatedFromBlock == blockSize)
				{
					//Allocate new block
					MemBlock * newBlock = reinterpret_cast<MemBlock *>(
									new char[sizeof(MemBlock *) + (blockSize * sizeof(T))]);

					//Set next block
					newBlock->nextBlock = headBlock;

					//Set this block as the current one
					headBlock = newBlock;
					allocatedFromBlock = 0;
				}

				//Allocate from block
				T * blockMemStart = reinterpret_cast<T *>(&headBlock->dummyMemStart);

				blockMemStart = blockMemStart + allocatedFromBlock;
				++allocatedFromBlock;

				return new (blockMemStart) T;
			}
		}

		//Frees 1 object which was allocated using Allocate
		// and calls its destructor
		void Free(T * object)
		{
			//Invoke destructor
			object->~T();

			//Convert object to chain item
			MemChain * newChainItem = reinterpret_cast<MemChain *>(object);

			//Add object to chain
			newChainItem->next = headChain;
			headChain = newChainItem;
		}
	};
}

#endif /* POOL_H_ */
