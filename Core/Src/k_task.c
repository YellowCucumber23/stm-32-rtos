#include "main.h"
#include "common.h"
#include "k_task.h"
#include "k_mem.h"

#include <stdlib.h>
#include <stdio.h>

uint8_t SVC_RET;
TCB* new_task;
int new_deadline = 5;
int timer = 1000;

void SVC_Handler_Main(unsigned int *svc_args)
{
	/*
	 * Stack contains:
	 * r0, r1, r2, r3, r12, r14, the return address and xPSR
	 * First argument (r0) is svc_args[0]
	 */
	unsigned int svc_number = ((char *)svc_args[6])[-2];

	switch (svc_number)
	{
	case SVC_KERNEL_START:
		current_task = pop_task();
		current_task->state = RUNNING;
		__set_PSP(current_task->stack_high);
		__run_first_thread();
		break;
	case SVC_KERNEL_YIELD:
		current_task->state = READY;
		current_task->time_left = current_task->deadline;
		queue_task(current_task);
		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; // Calling PendSV
		__asm("isb");
		break;
	case SVC_KERNEL_EXIT:
		current_task->state = DORMANT;
		stack_used -= current_task->stack_size;
		task_count--;
		k_mem_dealloc(current_task->stack_bot);
		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; // Calling PendSV
		__asm("isb");
		break;
	case SVC_TASK_CREATE:
		// Find an empty TCB in task_list
		// Start at index 1 since index 0 is reserved for null task
		TCB* real_cur_task = current_task;
		for (int i = 1; i < MAX_TASKS; ++i)
		{
			// If there is an empty entry in the TCB list a
			if (task_list[i].state == DORMANT || task_list[i].state == UNINIT)
			{
				init_tcb(i, new_task, new_deadline);
				queue_task(&task_list[i]);
				new_deadline = 5;

				// Make the task being created own stack initiziation
				current_task = &task_list[i];
				SVC_RET = init_t_stack(&task_list[i], new_task);
				current_task = real_cur_task;
				if (SVC_RET == RTX_ERR)
				{
					task_list[i].state = UNINIT;
					return;
				}

				if (task_list[i].time_left < current_task->time_left ||
						(task_list[i].time_left == current_task->time_left
								&& task_list[i].tid == current_task->tid))
				{
					current_task->state = READY;
					queue_task(current_task);
					SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; // Calling PendSV
					__asm("isb");
				}

				return;
			}
		}
		break;
	case SVC_KERNEL_OS_SLEEP:
		current_task->state = SLEEPING;
		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; // Calling PendSV
		__asm("isb");
		break;
	default:
		break;
	}
}

/********************
 * 					*
 * HELPER FUNCTIONS *
 * 					*
 ********************/

// Updates task_list array based on input index and stack high parameter.
void init_tcb(size_t idx, TCB *input, int deadline)
{
	// Update TCB within array
	task_list[idx].ptask = input->ptask;
	task_list[idx].stack_size = input->stack_size;
	task_list[idx].tid = idx;
	task_list[idx].state = READY;
	task_list[idx].deadline = deadline;
	task_list[idx].time_left = deadline;
	task_list[idx].sleep_time = 0;

	input->tid = idx;
	stack_used += input->stack_size;
	task_count++;
}

int init_t_stack(TCB *task, TCB *input)
{
	task->stack_bot = k_mem_alloc(task->stack_size);
	task->stack_high = task->stack_bot + task->stack_size;

	input->stack_bot = task->stack_bot;
	input->stack_high = task->stack_high;
	if (task->stack_bot == (uint32_t) NULL) { return RTX_ERR; }

	uint32_t *ptr = (uint32_t *)task->stack_high;
	*(--ptr) = 1 << 24;
	*(--ptr) = (uint32_t)task->ptask;
	for (int i = 0; i < 14; ++i)
	{
		*(--ptr) = 0xA;
	}
	task->stack_high = (uint32_t)ptr;
	return RTX_OK;
}

uint8_t heap_swap_check(TCB* parent, TCB* child)
{
	return (parent->time_left == child->time_left && parent->tid > child->tid)
			|| parent->time_left > child->time_left;
}

void queue_task(TCB *task)
{
	if (prio_q_size >= MAX_TASKS) {
		// TESTING CODE, WE SHOULD NEVER REACH HERE
		exit(1);
		return;
	}

	size_t cur = ++prio_q_size;
	task_prio_q[cur] = task;

	while (cur/2 > 0)
	{
		TCB* parent = task_prio_q[cur/2];
		TCB* child = task_prio_q[cur];

		if (heap_swap_check(parent, child)) {
			task_prio_q[cur/2] = child;
			task_prio_q[cur] = parent;
			cur = cur/2;
		} else {
			break;
		}
	}
	return;
}

TCB *pop_task()
{
	// Run the null task if nothing to queue
	if (prio_q_size == 0) {
		return &task_list[0];
	}

	TCB* top = task_prio_q[1];
	task_prio_q[1] = task_prio_q[prio_q_size];
	task_prio_q[prio_q_size--] = NULL;

	size_t cur = 1;
	while (cur*2 <= prio_q_size)
	{
		TCB* parent = task_prio_q[cur];
		TCB* left_c = task_prio_q[cur*2];
		TCB* right_c = task_prio_q[cur*2 + 1];

		if (left_c && heap_swap_check(parent, left_c)) {
			task_prio_q[cur*2] = parent;
			task_prio_q[cur] = left_c;
			cur = cur*2;
		} else if (right_c && heap_swap_check(parent, right_c)) {
			task_prio_q[cur*2 + 1] = parent;
			task_prio_q[cur] = right_c;
			cur = cur*2 + 1;
		} else {
			break;
		}
	}
	return top;
}

void update_heap(task_t TID) {
	// Find the index of the task to be heapified
	// Start at index 1 since index 0 is reserved for null task
	int task_index = -1;
	for (int i = 1; i <= prio_q_size; ++i) {
		if (task_prio_q[i]->tid == TID) {
			task_index = i;
			break;
		}
	}
	if (task_index == -1) {
		return; // Task not found in the queue
	}

	// Heapify-Up: Move node up if it has higher priority than its parent
    while (task_index > 1) {
        int parent_index = task_index / 2;
        if (!heap_swap_check(task_prio_q[parent_index], task_prio_q[task_index])) {
            break; // Heap property is satisfied
        }
        // Swap with parent
        TCB* temp = task_prio_q[parent_index];
        task_prio_q[parent_index] = task_prio_q[task_index];
        task_prio_q[task_index] = temp;
        task_index = parent_index;
    }

    // Heapify-Down: Move node down if it has lower priority than its children
    while (1) {
        int left_child_index = task_index * 2;
        int right_child_index = task_index * 2 + 1;
        int swap_index = task_index;

        // Find the highest-priority child to swap with
        if (left_child_index <= prio_q_size && heap_swap_check(task_prio_q[task_index], task_prio_q[left_child_index])) {
            swap_index = left_child_index;
        }
        if (right_child_index <= prio_q_size && heap_swap_check(task_prio_q[swap_index], task_prio_q[right_child_index])) {
            swap_index = right_child_index;
        }

        if (swap_index == task_index) {
            break; // Heap property is restored
        }

        // Swap and continue
        TCB* temp = task_prio_q[task_index];
        task_prio_q[task_index] = task_prio_q[swap_index];
        task_prio_q[swap_index] = temp;
        task_index = swap_index;
    }
}

int run_scheduler()
{
	// Account for R4-R11
	current_task->stack_high = (uint32_t *)(__get_PSP() - 8 * sizeof(uint32_t));

	// Set PSP to next task
	TCB *next_task = pop_task();
	if (next_task != NULL)
	{
		next_task->state = RUNNING;
		current_task = next_task;
		__set_PSP(next_task->stack_high);

		return RTX_OK;
	}
	else
	{
		return RTX_ERR;
	}
}

void tick_time_left()
{
	if(kernel_init) {
		uint8_t call_scheduler = 0;
		for (int i = 1; i < MAX_TASKS; i++) {
			if (task_list[i].state == READY || task_list[i].state == RUNNING) {
				task_list[i].time_left -= (task_list[i].time_left != 0);


				if (task_list[i].state == RUNNING && task_list[i].time_left == 0)
				{
					task_list[i].time_left = task_list[i].deadline;
					call_scheduler = 1;
				}
			} else if (task_list[i].state == SLEEPING) {
				task_list[i].sleep_time--;
				if (task_list[i].sleep_time == 0)
				{
					task_list[i].state = READY;
					task_list[i].time_left = task_list[i].deadline;
					queue_task(&task_list[i]);

					call_scheduler = 1;
				}
			}
		}

		if (call_scheduler) {
			current_task->state = READY; //Set current task to ready
			if (current_task->tid != TID_NULL) queue_task(current_task);
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; // Calling PendSV
			__asm("isb");
		}
	}
}

void null_task(void*)
{
	while(1);
}

/********************
 * 					*
 * LAB FUNCTIONS	*
 * 					*
 ********************/

// define globals here
TCB task_list[MAX_TASKS]; // Static array of TCB's
uint8_t task_count = 0;
uint16_t stack_used = 0;
TCB *current_task = NULL;
uint8_t kernel_init = 0;

TCB* task_prio_q[MAX_TASKS+1];
uint8_t prio_q_size = 0;

void osKernelInit()
{
	SHPR3 = (SHPR3 & ~(0xFFU << 24)) | (0xF0U << 24); // SysTick is lowest priority (highest number)
	SHPR3 = (SHPR3 & ~(0xFFU << 16)) | (0xE0U << 16); // PendSV is in the middle

	SHPR2 = (SHPR2 & ~(0xFFU << 24)) | (0xD0U << 24); // SVC is highest priority (lowest number)

	kernel_init = 1;
	k_mem_init();

	// Initialize the NULL task TCB
	task_list[0].ptask = &null_task;
	task_list[0].stack_size = MIN_STACK_SIZE;
	task_list[0].tid = TID_NULL;
	task_list[0].state = READY;

	task_list[0].stack_bot = k_mem_alloc(task_list[0].stack_size);
	task_list[0].stack_high = task_list[0].stack_bot + task_list[0].stack_size;

	uint32_t *ptr = (uint32_t *)task_list[0].stack_high;
	*(--ptr) = 1 << 24;
	*(--ptr) = (uint32_t)task_list[0].ptask;
	for (int i = 0; i < 14; ++i)
	{
		*(--ptr) = 0xA;
	}
	task_list[0].stack_high = (uint32_t)ptr;

	// Initialize all other task TCB
	for (int i = 1; i < MAX_TASKS; ++i)
	{
		task_list[i].ptask = NULL;
		task_list[i].stack_high = 0;
		task_list[i].stack_bot = 0;
		task_list[i].tid = i;
		task_list[i].state = UNINIT;
		task_list[i].stack_size = 0;
	}

	task_count = 1; /* 1 since NULL_TASK already in task list */
	current_task = NULL;

	for (int i = 0; i < MAX_TASKS; ++i) task_prio_q[i] = NULL;
	prio_q_size = 0;
}

int osKernelStart()
{
	if (task_count == 0)
	{
		return RTX_ERR;
	}
	if (current_task)
	{
		return RTX_ERR;
	}

	__asm("SVC #1");
}

void osYield()
{
	__asm("SVC #2");
}

int osCreateTask(TCB *task)
{
	if (task_count >= MAX_TASKS)
	{
		return RTX_ERR;
	}
	if ((task->stack_size % 8) != 0)
	{
		return RTX_ERR;
	}

	new_task = task;
	__asm("SVC #5");
	return SVC_RET;
}

int osTaskInfo(task_t TID, TCB *task_copy)
{
	// Check if TID is valid/exists
	if (task_list[TID].tid == TID_NULL || TID >= MAX_TASKS)
	{
		return RTX_ERR;
	}

	// Fill task_copy with task info
	task_copy->ptask = task_list[TID].ptask;
	task_copy->stack_high = task_list[TID].stack_high;
	task_copy->stack_bot = task_list[TID].stack_bot;
	task_copy->stack_size = task_list[TID].stack_size;
	task_copy->tid = task_list[TID].tid;
	task_copy->state = task_list[TID].state;

	task_copy->deadline = task_list[TID].deadline;
	task_copy->time_left = task_list[TID].time_left;
	task_copy->sleep_time = task_list[TID].sleep_time;

	return RTX_OK;
}

task_t osGetTID(void)
{
	if (current_task == NULL)
	{
		return 0;
	}
	return current_task->tid;
}

int osTaskExit(void)
{
	__asm("SVC #3");

	return RTX_OK;
}

void osSleep(int timeInMs)
{
	current_task->sleep_time = timeInMs;
	__asm("SVC #6");
}

void osPeriodYield(void) {
	current_task->sleep_time = current_task->time_left;
	__asm("SVC #6");
}

int osSetDeadline(int deadline, task_t TID) {
	// Return ERR if deadline is negative, TID is invalid, TID is the current task, or task is not in the READY state
	if (deadline < 0 || 
		TID >= MAX_TASKS || 
		TID == TID_NULL || 
		TID == current_task->tid || 
		task_list[TID].tid == TID_NULL ||
		task_list[TID].state != READY) {
		return RTX_ERR;
	}
	
	// Block interrupts
	__disable_irq();

	task_list[TID].deadline = deadline;
	task_list[TID].time_left = deadline;
	
	// Will need to remove task from queue and reinsert it to maintain priority
	update_heap(TID);

	// Must check if TID task needs to be moved in the queue (preempted)
	int preempt = 0;

    if (prio_q_size > 0 && 
        (task_prio_q[1]->time_left < current_task->time_left || 
        (task_prio_q[1]->time_left == current_task->time_left && 
         task_prio_q[1]->tid < current_task->tid))) {
        preempt = 1;
    }

	// Enable interrupts
	__enable_irq();

	// If it does need to be preempted, trigger context switch
	// Same as for Kernel Yield? Copied down, without the deadline check
	if (preempt) {
		current_task->state = READY;
		queue_task(current_task);
		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; // Calling PendSV
		__asm("isb");
	}
	return RTX_OK;
}

int osCreateDeadlineTask(int deadline, TCB* task)
{
	if (task_count >= MAX_TASKS)
	{
		return RTX_ERR;
	}
	if ((task->stack_size % 8) != 0)
	{
		return RTX_ERR;
	}
	if (deadline <= 0)
	{
		return RTX_ERR;
	}

	new_task = task;
	new_deadline = deadline;
	__asm("SVC #5");
	return SVC_RET;
}
