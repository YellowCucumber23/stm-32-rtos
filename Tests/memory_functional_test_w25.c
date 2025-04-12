#include "main.h"
#include <stdio.h>
#include "common.h"
#include "k_task.h"
#include "k_mem.h"
#include <stdlib.h> //for testing
#include "string.h" //for testing

#define ITERATIONS 100
uint8_t s_data_string[1000] = "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligula eget dolor. Aenean massa. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Donec quam felis, ultricies nec, pellentesque eu, pretium quis, sem. Nulla consequat massa quis enim. Donec pede justo, fringilla vel, aliquet nec, vulputate eget, arcu. In enim justo, rhoncus ut, imperdiet a, venenatis vitae, justo. Nullam dictum felis eu pede mollis pretium. Integer tincidunt. Cras dapibus. Vivamus elementum semper nisi. Aenean vulputate eleifend tellus. Aenean leo ligula, porttitor eu, consequat vitae, eleifend ac, enim. Aliquam lorem ante, dapibus in, viverra quis, feugiat a, tellus. Phasellus viverra nulla ut metus varius laoreet. Quisque rutrum. Aenean imperdiet. Etiam ultricies nisi vel augue. Curabitur ullamcorper ultricies nisi. Nam eget dui. Etiam rhoncus. Maecenas tempus, tellus eget condimentum rhoncus, sem quam semper libero, sit amet adipiscing sem neque sed ipsum. N";
uint8_t* p_buffers[ITERATIONS];
uint8_t i_checksums[ITERATIONS];
uint32_t i_buffer_sizes[ITERATIONS];
uint32_t i_total_alloc_bytes = 0;

uint8_t CalcChecksum(uint8_t* p_buffer, uint32_t i_buffer_size){
	uint8_t checksum = 0;
	for (int i = 0; i < i_buffer_size; i++){
		checksum = checksum ^ p_buffer[i];
	}
	return checksum;
}


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

  uint32_t size, r;
  uint32_t cnt_alloc_fails = 0;
  uint32_t cnt_dealloc_fails = 0;
  uint32_t cnt_checksum_fails = 0;

  printf("\r\n\r\n");
  printf("Starting testing ================================\r\n");

  for (int i = 0; i < ITERATIONS; i++ ){

	  if (i_total_alloc_bytes < 0x8000) { //proceed if we are still under 32KB of allocated bytes (most projects seem to have > 40KB of free space)
		  size = rand() % 1000; //select a random buffer size
		  p_buffers[i] = k_mem_alloc(size); //allocate buffer
		  printf("itr=%d, alloc %lu bytes, ptr=%p\r\n", i, size, p_buffers[i]);

		  if (p_buffers[i] != NULL){ //success
			  i_buffer_sizes[i] = size; //keep track of this allocation
			  i_checksums[i] = CalcChecksum(s_data_string, size); //calculate checksum on original data, for the portion that will fit into this buffer
			  memcpy(p_buffers[i], s_data_string, size); //fill the buffer fully with data (memcpy function used for testing only)
			  i_total_alloc_bytes = i_total_alloc_bytes + size;
		  } else { //did not allocate (should be due to fragmentation)
			  printf("NULL POINTER (allocation failed)\r\n\r\n");
			  cnt_alloc_fails = cnt_alloc_fails + 1;
			  i_buffer_sizes[i]=0;
		  }
	  } else { //skip this iteration if we have used up >= 32KB of the heap
				//hence the only legitimate reason alloc might fail is fragmentation

		  i_buffer_sizes[i] = 0;
		  p_buffers[i] = NULL;
		  //printf("itr=%d, skipped (%lu bytes used up)\r\n", i, i_total_alloc_bytes);
		}

	  if (i > 0 && i % 2 == 0){ //deallocate something on every other iteration
		  do{
			  r = rand() % i;
		  } while(i_buffer_sizes[r] == 0);//repeat until finding a buffer that still exists (not deallocated already)

		  printf("dealloc mem from itr %lu, ptr=%p, ~%lu bytes\r\n", r, p_buffers[r], i_buffer_sizes[r]);
		  if (k_mem_dealloc(p_buffers[r]) != RTX_ERR) { //success
			  i_total_alloc_bytes = i_total_alloc_bytes - i_buffer_sizes[r];
			  i_buffer_sizes[r] = 0; //keep track of the dealloc
			  p_buffers[r] = NULL;
		  }else{
			  printf("RTX_ERR (deallocation failed)\r\n\r\n");
			  cnt_dealloc_fails += 1;
		  }
	  }
  }

  printf("Validating buffer contents... \r\n");

  uint8_t checksum;
  for (int i = 0; i < ITERATIONS; i++ ){
	  if (i_buffer_sizes[i] > 0){
		  checksum = CalcChecksum(p_buffers[i], i_buffer_sizes[i]);
		  if (checksum != i_checksums[i]){
			  cnt_checksum_fails += 1;
			  printf("buffer from itr %d corrupted, checksum=%u, expected=%u \r\n\r\n", i, checksum, i_checksums[i]);
		  }
	  }
  }

  printf("Total corrupted buffers = %lu \r\n", cnt_checksum_fails);
  printf("Total failed allocs = %lu \r\n", cnt_alloc_fails);
  printf("Total failed deallocs = %lu \r\n", cnt_dealloc_fails);


  printf("back to main\r\n");
  while (1);
 }

