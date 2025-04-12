#include "main.h"
#include <stdio.h>
#include "common.h"
#include "k_task.h"
#include "k_mem.h"

#define  ARM_CM_DEMCR      (*(uint32_t *)0xE000EDFC)
#define  ARM_CM_DWT_CTRL   (*(uint32_t *)0xE0001000)
#define  ARM_CM_DWT_CYCCNT (*(uint32_t *)0xE0001004)




int main(void)
{

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

  if (ARM_CM_DWT_CTRL != 0) {        // See if DWT is available
	  printf("Using DWT\r\n\r\n");
      ARM_CM_DEMCR      |= 1 << 24;  // Set bit 24
      ARM_CM_DWT_CYCCNT  = 0;
      ARM_CM_DWT_CTRL   |= 1 << 0;   // Set bit 0
  }else{
	  printf("DWT not available \r\n\r\n");
  }

  int N = 100;
  volatile U32* p_temp;
  uint32_t timestamps[N+1];

//System under test---------------------
  //measure time to allocate N block
  for (int i = 0; i < N; i ++){
	  timestamps[i] = ARM_CM_DWT_CYCCNT;
	  p_temp = (U32*)k_mem_alloc(4);
  }
  timestamps[N] = ARM_CM_DWT_CYCCNT;
  //print total clock ticks as well as ticks per iteration
  printf("k_mem_alloc time: %lu\r\n", timestamps[N] - timestamps[0]);
  printf("Time per iteration:\r\n");
  for (int i = 0; i < N; i ++){
	  printf("%u, ", timestamps[i+1] - timestamps[i]);
  }
  printf("\r\n\r\n");

/*
//Compiler's version-------------------
  //measure time to allocate N block
  for (int i = 0; i < N; i ++){
	  timestamps[i] = ARM_CM_DWT_CYCCNT;
	  p_temp = (U32*)malloc(4);
  }
  timestamps[N] = ARM_CM_DWT_CYCCNT;
  //print total clock ticks as well as ticks per iteration
  printf("malloc time: %lu\r\n", timestamps[N] - timestamps[0]);
  printf("Time per iteration:\r\n");
  for (int i = 0; i < N; i ++){
	  printf("%u, ", timestamps[i+1] - timestamps[i]);
  }
  printf("\r\n\r\n");
*/
  
  printf("back to main\r\n");
  while (1);
 }

