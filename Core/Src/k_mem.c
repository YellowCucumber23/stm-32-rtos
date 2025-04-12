#include "main.h"
#include "common.h"
#include "k_mem.h"

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

uint8_t heap_init = 0;
heap_block_t* free_list = NULL;
heap_block_t* use_list = NULL;

heap_block_t* k_mem_pop_free(heap_block_t* current, size_t aligned_size)
{
	// heap_block_t should always be in sizes of 4 bytes.
	size_t split_size = aligned_size + sizeof(heap_block_t);

    // Update current block metadata
    // Get current task ID or kernel for ownership
    current->tid = (current_task->tid != TID_NULL) ? current_task->tid : TID_NULL;
    current->status = OCCUPIED;

	if (current->aligned_size >= split_size + MIN_BLOCK_SIZE) {
	    // Split the block if it is larger than a new block + min block size
		// We do this so that we only split a node if we could have used
		// extra space for more allocations.

		heap_block_t* new_free = (heap_block_t*) ((uint32_t) current->start + aligned_size);

		// do stuff with metadata
		new_free->tid = TID_INVALID;
		new_free->start = (uint32_t*) ((uint32_t) current->start + split_size);
		new_free->status = FREE;

		// Update size to aligned
		new_free->aligned_size = current->aligned_size - split_size;
		current->aligned_size = aligned_size;

		// Updating pointers in the free list
		new_free->next = current->next;
		new_free->prev = current->prev;
		if (new_free->next != NULL) {
			new_free->next->prev = new_free;
		}
		if (new_free->prev != NULL) {
			new_free->prev->next = new_free;
		}
		// Update free list head
		if (current == free_list) {
			free_list = new_free;
		}

		// Update pointers for memory list
		new_free->next_block = current->next_block;
		new_free->prev_block = current;
		if (new_free->next_block != NULL) {
			new_free->next_block->prev_block = new_free;
		}
		if (new_free->prev_block != NULL) {
			new_free->prev_block->next_block = new_free;
		}

	} else {
		// If the block is exactly the size we need, remove the block from the free list

		// Remove current from free list
		if (current->next != NULL) {
			current->next->prev = current->prev;
		}
		if (current->prev != NULL) {
			current->prev->next = current->next;
		}
		if (current == free_list) {
			free_list = current->next;
		}
	}

	return current;
}

/***********************************************************************************************
 * FUNCTION DEFINITIONS
 ***********************************************************************************************/
int k_mem_init()
{
    // Exit if kernel not initialized or heap already initialized
    if (kernel_init == 0 || heap_init == 1) { return RTX_ERR; }

    // Populate the free_list with the size of the entire heap
    free_list = (heap_block_t*) HEAP_START;
    free_list->tid = TID_INVALID;
    free_list->start = (uint32_t*) (HEAP_START + sizeof(heap_block_t));
    free_list->aligned_size = HEAP_END - HEAP_START - sizeof(heap_block_t);
    free_list->status = FREE;
    free_list->next_block = NULL;
    free_list->prev_block = NULL;
    free_list->next = NULL;
    free_list->prev = NULL;
    
    heap_init = 1;
    return RTX_OK;
}

void* k_mem_alloc(size_t size)
{
    // Return NULL if heap not initialized, or size is invalid.
    if (heap_init == 0 || size == 0 || size > HEAP_SIZE - sizeof(heap_block_t)) {
        return NULL;
    }

    // Find total size and align it to 4 bytes.
    // Add 3 to size, then sets last two bits to 0 aligning to 4 bits.
	size_t aligned_size = (size + 3) & ~3;

    // Iterate through list until we find a free block
    heap_block_t* current = free_list;
    while (current != NULL) {
    	// Found a block that is big enough
        if (current->aligned_size >= aligned_size) {
            current = k_mem_pop_free(current, aligned_size);

            // Add current to front of use list
            current->prev = NULL;
            current->next = use_list;
            if (use_list) { use_list->prev = current; }
            use_list = current;

            // Return the start of the block
            return current->start;
        }

        current = current->next;
    }

    return NULL;
}

// void k_mem_print() {
// 	heap_block_t* mem = (heap_block_t*) HEAP_START;
// 	uint32_t size = 0;
// 	while (mem) {
// 		printf("mem %s at %p : start = %p, aligned size is %lu, should go to %p\r\n", (mem->status == FREE)?"FREE":"OCCUPIED", mem, mem->start, mem->aligned_size, (uint32_t)mem->start + mem->aligned_size);
// 		size += mem->aligned_size + sizeof(heap_block_t);
// 		mem = mem->next_block;
// 	}
// 	printf("final size is at %lu\r\n", size);
// }

int k_mem_dealloc(void* ptr)
{
	if (!ptr) {
		return RTX_ERR;
	}

    task_t tid = (current_task != TID_NULL) ? current_task->tid : TID_NULL;

    heap_block_t* block = use_list;
    while (block) {
    	if (block->status == OCCUPIED && block->start == ptr && block->tid == tid) {
    		// Remove block from used list
    		if (block->prev) 		{ block->prev->next = block->next; }
    		if (block->next) 		{ block->next->prev = block->prev; }
    		if (block == use_list) 	{ use_list = block->next; }

    		block->status = FREE;

    		heap_block_t* prev = block->prev_block;
    		heap_block_t* next = block->next_block;

    		uint8_t prev_coalescing = (prev && prev->status == FREE);
    		uint8_t next_coalescing = (next && next->status == FREE);

    		// If theres no coalescing, update free list and exit
    		if (!prev_coalescing && !next_coalescing) {
        		// Edge case: free_list is empty
        		if (!free_list) {
        			block->prev = NULL;
					block->next = NULL;
        			free_list = block;
        		}
    			// Edge case: if block starts before free_list
        		else if (block->start < free_list->start) {
    				block->prev = NULL;
    				block->next = free_list;
    				free_list->prev = block;
    				free_list = block;
    			} else {
    				// Find from free list block of memory that starts after block
        			heap_block_t* free_mem = free_list;
					while (free_mem->next && free_mem->next->start < block->start) {
						free_mem = free_mem->next;
					}

					block->prev = free_mem;
					block->next = free_mem->next;
					if (free_mem->next) { free_mem->next->prev = block; }
					free_mem->next = block;
    			}

    			return RTX_OK;
    		}

    		if (prev_coalescing) {
    			prev->aligned_size += block->aligned_size + sizeof(heap_block_t);

				// Update mem_list
				prev->next_block = block->next_block;
				if (block->next_block) { block->next_block->prev_block = prev; }

				// Prev_block is already added to the free list, no need to update prev/next.
				block = prev;
			}
    		if (next_coalescing) {
    			block->aligned_size += next->aligned_size + sizeof(heap_block_t);

    			// Update mem_list
    			block->next_block = next->next_block;
				if (next->next_block) { next->next_block->prev_block = block; }

				// Update free list
				block->next = next->next;
				if (next->next) { next->next->prev = block; }

				// Only update prev when we couldn't coalesce prev_block
				if (!prev_coalescing) {
					block->prev = next->prev;
					if (next->prev) { next->prev->next = block; }
					if (next == free_list) { free_list = block; }
				}
    		}

    		return RTX_OK;
    	}

    	block = block->next;
    }

    return RTX_ERR;
}

int k_mem_count_extfrag(size_t size) {
    // returns the number of free memory regions strictly less than size, including the size of the data structure
    int count = 0;
    heap_block_t* current = free_list;
    while (current != NULL) {
        if ((current->aligned_size + sizeof(heap_block_t)) < size) {
            count++;
        }
        current = current->next;
    }
    return count;
}
