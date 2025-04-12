#include "common.h"
#include "k_task.h"
#include "k_mem.h"
#include "main.h"
#include <stdio.h>
int i_test = 0;
void Task1(void *) {
   while(1){
     printf("1\r\n");
     for (int i_cnt = 0; i_cnt < 5000; i_cnt++);
     osPeriodYield();
   }
}
void Task2(void *) {
   while(1){
     printf("2\r\n");
     for (int i_cnt = 0; i_cnt < 55000; i_cnt++);
     osPeriodYield();
   }
}
void Task3(void *) {
   while(1){
     printf("3\r\n");
     for (int i_cnt = 0; i_cnt < 5000; i_cnt++);
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
  k_mem_init();
  TCB st_mytask;
	st_mytask.stack_size = 0x400;
	st_mytask.ptask = &Task1;
	osCreateDeadlineTask(4, &st_mytask);
	st_mytask.ptask = &Task2;
	osCreateDeadlineTask(11, &st_mytask);
//	st_mytask.ptask = &Task3;
//	osCreateTask(&st_mytask);
  osKernelStart();
  while (1);
}
