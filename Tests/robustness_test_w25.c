#include "common.h"
#include "k_task.h"
#include "main.h"
#include <stdio.h>


int i_test = 0;

void Task1(void *) {
	i_test++;
	osYield();

	//instead of a while loop, keep recreating itself and exiting
	TCB st_mytask;
	st_mytask.ptask = &Task1;
	st_mytask.stack_size = 0x400;
	osCreateTask(&st_mytask);
	osTaskExit();
}

void Task2(void *) {
	while(1){
		printf("Back to you %d\r\n",i_test);
		osYield();
	}
}

int main(void) {
  /* MCU Configuration: Don't change this or the whole chip won't work!*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  /* MCU Configuration is now complete. Start writing your code below this line */

  osKernelInit();

  TCB st_mytask;
  st_mytask.stack_size = 0x400;

  st_mytask.ptask = &Task1;
  osCreateTask(&st_mytask);

  st_mytask.ptask = &Task2;
  osCreateTask(&st_mytask);

  osKernelStart();

  while (1);
}
