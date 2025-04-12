  .syntax unified
  .cpu cortex-m4
  .fpu softvfp
  .thumb

.global SVC_Handler
.thumb_func
SVC_Handler:
	TST lr, #4
	ITE EQ
	MRSEQ r0, MSP
	MRSNE r0, PSP
	B SVC_Handler_Main

.global __run_first_thread
.thumb_func
__run_first_thread:
	POP {R7}
	POP {R7}
	MRS R0, PSP
  	LDMIA R0!,{R4-R11}
  	MSR PSP, R0
  	MOV LR, #0xFFFFFFFD
  	BX LR

.global PendSV_Handler
.thumb_func
PendSV_Handler:
//	POP {R7}
//	POP {R7}

	MRS R0, PSP
  	STMDB R0!,{R4-R11}

	BL run_scheduler
  	MRS R0, PSP

	LDMIA R0!,{R4-R11}
  	MOV LR, #0xFFFFFFFD
	MSR PSP, R0
  	BX LR
