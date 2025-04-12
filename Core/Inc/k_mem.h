/*
 * k_mem.h
 *
 *  Created on: Jan 5, 2024
 *      Author: nexususer
 *
 *      NOTE: any C functions you write must go into a corresponding c file that you create in the Core->Src folder
 */
#include <stdio.h>
#include "common.h"

#ifndef INC_K_MEM_H_
#define INC_K_MEM_H_

#define HEAP_START (uint32_t) &_img_end
#define HEAP_END ((uint32_t) &_estack - 0x4000)
#define HEAP_SIZE HEAP_END - HEAP_START 

#define MIN_BLOCK_SIZE 4

extern uint8_t heap_init;
extern uint32_t _img_end;
extern uint32_t _estack;

/**
 * @brief Use the current block to create a node of exact given size.
 * @return heap_block_t* NULL on failure, returns current on success.
 */
heap_block_t* k_mem_pop_free(heap_block_t* current, size_t aligned_size);

/**
 * @brief Initialize the memory manager
 * @return int RTX_OK on success, RTX_ERR on failure
 */
int k_mem_init();

/**
 * @brief Allocate a heap block aligned to 4 bytes
 * @param size The size of the heap block
 * @return void* Pointer to start of usable memory after metadata
 */
void* k_mem_alloc(size_t size);

/**
 * @brief Free memory pointed to by ptr as long as currently running task owns the block pointed to by ptr
 * @param ptr Pointer to the block of memory
 * @return int RTX_OK on success or ptr is NULL and RTX_ERR on failure (current task does not own block)
 */
int k_mem_dealloc(void* ptr);

/**
 * @brief Count the number of free memory regions strictly less than size
 * @param size The size of the blocks
 * @return int The number of blocks less than size
 */
int k_mem_count_extfrag(size_t size);

#endif /* INC_K_MEM_H_ */
