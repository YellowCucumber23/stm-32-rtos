/*
 * common.h
 *
 *  Created on: Jan 5, 2024
 *      Author: nexususer
 *
 *      NOTE: If you feel that there are common
 *      C functions corresponding to this
 *      header, then any C functions you write must go into a corresponding c file that you create in the Core->Src folder
 */
#include <stdint.h>

#ifndef INC_COMMON_H_
#define INC_COMMON_H_

#define TID_NULL 0 //predefined Task ID for the NULL task
#define TID_INVALID -1  //Invalid TID
#define MAX_TASKS 16 //maximum number of tasks in the system
#define MAIN_STACK_SIZE 0x400 // size of OS stack
#define THREAD_STACK_SIZE 0x400 // size of thread's stack
#define MAX_STACK_SIZE 0x4000
#define MIN_STACK_SIZE 0x200

#define RTX_ERR 1
#define RTX_OK 0

typedef unsigned int task_t;
extern int timer;

/***********************************************************************************************
 * HEAP DATA TYPES
 ***********************************************************************************************/
typedef enum heap_status{
    OCCUPIED,
    FREE
} heap_status_t;

typedef struct heap_block{
    task_t tid;                     // ID of the task this block belongs to
    uint32_t* start;                // Address of start of heap block
    uint32_t aligned_size;          // Actual size of allocated heap aligned to 4 bytes
    heap_status_t status;
    struct heap_block *next;        // Pointer to the next heap block in the free/use list
    struct heap_block *prev;        // Pointer to the prev. heap block in the free/use list
    struct heap_block *next_block;  // Pointer to the next heap block in physical memory
    struct heap_block *prev_block;  // Pointer to the prev. heap block in physical memory
} heap_block_t;

extern heap_block_t* free_list; 	// Kept in order, ascending mem. addr.
extern heap_block_t* use_list;		// Kept non-ordered.


/***********************************************************************************************
 * TASK DATA TYPES
 ***********************************************************************************************/
typedef enum {
    UNINIT 		= -1,
    DORMANT 	= 0,	//state of terminated task
    READY 		= 1,  	//state of task that can be scheduled but is not running
    RUNNING 	= 2,  	//state of running task
    SLEEPING 	= 3  	//state of sleeping task
} state_t;

typedef struct task_control_block {
    void (*ptask)(void* args); //entry address
    uint16_t stack_size; //stack size. Must be a multiple of 8
	uint32_t stack_high; //where stack left off (high address)
	uint32_t stack_bot;  //starting address of stack (low address)

    task_t tid; //task ID

    state_t state; //task's state

    uint32_t deadline; // how much time was scheduled (in ms)
    uint32_t time_left; // how much time from deadline been used (in ms)
    uint32_t sleep_time;
}  TCB;

extern uint8_t kernel_init;
extern TCB task_list[MAX_TASKS]; // Static array of TCB's
extern uint8_t task_count;       // Current number of tasks
extern uint16_t stack_used;      // How much of the stack is used
extern TCB* current_task;        // Current executing task

extern TCB* task_prio_q[MAX_TASKS+1]; // Static array of TCBs that are queued, organized as min heap
extern uint8_t prio_q_size;

#endif /* INC_COMMON_H_ */
