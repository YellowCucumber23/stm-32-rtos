void Task1(void *) {
   while(1){
     printf("1\r\n");
     for (i_cnt = 0; i_cnt < 5000; i_cnt++);
     osYield();
   }
}


void Task2(void *) {
   while(1){
     printf("2\r\n");
     for (i_cnt = 0; i_cnt < 5000; i_cnt++);
     osYield();
   }
}


void Task3(void *) {
   while(1){
     printf("3\r\n");
     for (i_cnt = 0; i_cnt < 5000; i_cnt++);
     osYield();
   }
}

//in main

TCB st_mytask;
  st_mytask.stack_size = STACK_SIZE;
  st_mytask.ptask = &Task1;
  osCreateTask(&st_mytask);


  st_mytask.ptask = &Task2;
  osCreateTask(&st_mytask);


  st_mytask.ptask = &Task3;
  osCreateTask(&st_mytask);