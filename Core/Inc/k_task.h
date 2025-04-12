/*
 * k_task.h
 *
 *  Created on: Jan 5, 2024
 *      Author: nexususer
 *
 *      NOTE: any C functions you write must go into a corresponding c file that you create in the Core->Src folder
 */

#include <stdio.h>
#include "common.h"

#ifndef INC_K_TASK_H_
#define INC_K_TASK_H_

#define SVC_KERNEL_START 1
#define SVC_KERNEL_YIELD 2
#define SVC_KERNEL_EXIT  3
#define SVC_TASK_CREATE 5
#define SVC_KERNEL_OS_SLEEP 6

#define SHPR2 (*((volatile uint32_t*)0xE000ED1C))//SVC is bits 31-28
#define SHPR3 (*((volatile uint32_t*)0xE000ED20))//SysTick is bits 31-28, and PendSV is bits 23-20

#define __set_pendsv() SCB->ICSR = 0x10000000

extern uint8_t SVC_RET;
extern TCB* new_task;

/**
 * @brief SVC syscall handler
 * @param svc_args The system call number
 */
void SVC_Handler_Main(unsigned int*);

/**
 * @brief Assembly function to start up the first thread
 */
void __run_first_thread();

/**
 * @brief Context switch to another task
 * @return int RTX_OK if successful and RTX_ERROR otherwise
 */
int run_scheduler();

/**
 * @brief Initialize a task control block
 * @param idx Index in task array
 * @param input Reference TCB
 * @param stack_high High address of the task stack (start of stack)
 */
void init_tcb(size_t idx, TCB *input, int deadline);

/**
 * @brief Initialize a task stack
 */
int init_t_stack(TCB *task, TCB *input);

/**
 * @brief Insert a task into the queue
 * @param task Pointer to the task control block that needs to be added
 */
void queue_task(TCB* task);

/**
 * @brief Remove a task from the queue
 * @return TCB* Pointer to the removed task control block
 */
TCB* pop_task();

uint8_t heap_swap_check(TCB* parent, TCB* child);

void update_heap(task_t TID);

void tick_time_left();

void null_task(void*);

/**
 * @brief Initializes all global kernel-level data structures and other variables.
 *        This function must be called before any other RTX functions will work
 */
void osKernelInit(void);

/**
 * @brief Create a new task and register it with the RTX if possible.
 *        It is expected that the application code sets up the relevant TCB fields before
 *        calling this function
 * @param task The task to add to the RTX
 * @return int RTX_OK on success and RTX_ERR on failure
 */
int osCreateTask(TCB* task);

/**
 * @brief Called by application code after the kernel has been initialized to run the first task
 * @return int RTX_ERR if the kernel is not initialized or if the kernel is already running.
 *         Returns nothing otherwise
 */
int osKernelStart(void);

/**
 * @brief Halts the execution of one task, saves it contexts, runs the
 *        scheduler, and loads the context of the next task to run
 *
 */
void osYield(void);

/**
 * @brief Copy over a task from a given TID
 * @param TID The task to copy
 * @param task_copy Destination of copied task
 * @return int RTX_OK if a task with the given TID exists, and RTX_ERR otherwise
 */
int osTaskInfo(task_t TID, TCB* task_copy);

/**
 * @brief Enables a task to find out its own TID. It is intended to be used by the user application.
 * @return task_t TID of the currently running user task. It returns 0 if the Kernel has not started
 */
task_t osGetTID(void);

/**
 * @brief Exit task and anything being used is returned to OS
 * @return int Nothing if successful, RTX_ERR otherwise
 */
int osTaskExit(void);

void osSleep(int timeInMs);

void osPeriodYield(void);

int osSetDeadline(int deadline, task_t TID);

int osCreateDeadlineTask(int deadline, TCB* task);

#endif /* INC_K_TASK_H_ */
