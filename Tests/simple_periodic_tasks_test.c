int i_test = 0;

int i_test2 = 0;


void TaskA(void *) {
   while(1){
      printf("%d, %d\r\n", i_test, i_test2);
      osPeriodYield();
   }
}

void TaskB(void *) {
   while(1){
      i_test = i_test + 1;
      osPeriodYield();
   }
}

void TaskC(void *) {
   while(1){
      i_test2 = i_test2 + 1;
      osPeriodYield();
   }
}


//in main

  TCB st_mytask;
  st_mytask.stack_size = STACK_SIZE;
  st_mytask.ptask = &TaskA;
  osCreateDeadlineTask(4, &st_mytask);

  st_mytask.ptask = &TaskB;
  osCreateDeadlineTask(4, &st_mytask); 

  st_mytask.ptask = &TaskC;
  osCreateDeadlineTask(12, &st_mytask);