.equ REGBYTES, 4
.equ FRAME_SIZE, 112

.macro M_PSP_PUSH
  addi    sp,sp,-FRAME_SIZE
  sw  ra,REGBYTES*27(sp)
  sw  t0,REGBYTES*26(sp)
  sw  t1,REGBYTES*25(sp)
  sw  t2,REGBYTES*24(sp)
  sw  a0,REGBYTES*23(sp)
  sw  a1,REGBYTES*22(sp)
  sw  a2,REGBYTES*21(sp)
  sw  a3,REGBYTES*20(sp)
  sw  a4,REGBYTES*19(sp)
  sw  a5,REGBYTES*18(sp)
  sw  a6,REGBYTES*17(sp)
  sw  a7,REGBYTES*16(sp)
  sw  t3,REGBYTES*15(sp)
  sw  t4,REGBYTES*14(sp)
  sw  t5,REGBYTES*13(sp)
  sw  t6,REGBYTES*12(sp)
  sw  s0,REGBYTES*11(sp)
  sw  s1,REGBYTES*10(sp)
  sw  s2,REGBYTES*9(sp)
  sw  s3,REGBYTES*8(sp)
  sw  s4,REGBYTES*7(sp)
  sw  s5,REGBYTES*6(sp)
  sw  s6,REGBYTES*5(sp)
  sw  s7,REGBYTES*4(sp)
  sw  s8,REGBYTES*3(sp)
  sw  s9,REGBYTES*2(sp)
  sw  s10,REGBYTES*1(sp)
  sw  s11,REGBYTES*0(sp)
.endm

.macro M_PSP_POP
  lw  ra,REGBYTES*27(sp)
  lw  t0,REGBYTES*26(sp)
  lw  t1,REGBYTES*25(sp)
  lw  t2,REGBYTES*24(sp)
  lw  a0,REGBYTES*23(sp)
  lw  a1,REGBYTES*22(sp)
  lw  a2,REGBYTES*21(sp)
  lw  a3,REGBYTES*20(sp)
  lw  a4,REGBYTES*19(sp)
  lw  a5,REGBYTES*18(sp)
  lw  a6,REGBYTES*17(sp)
  lw  a7,REGBYTES*16(sp)
  lw  t3,REGBYTES*15(sp)
  lw  t4,REGBYTES*14(sp)
  lw  t5,REGBYTES*13(sp)
  lw  t6,REGBYTES*12(sp)
  lw  s0,REGBYTES*11(sp)
  lw  s1,REGBYTES*10(sp)
  lw  s2,REGBYTES*9(sp)
  lw  s3,REGBYTES*8(sp)
  lw  s4,REGBYTES*7(sp)
  lw  s5,REGBYTES*6(sp)
  lw  s6,REGBYTES*5(sp)
  lw  s7,REGBYTES*4(sp)
  lw  s8,REGBYTES*3(sp)
  lw  s9,REGBYTES*2(sp)
  lw  s10,REGBYTES*1(sp)
  lw  s11,REGBYTES*0(sp)
 addi    sp,sp,FRAME_SIZE
.endm

.section  .text
.global context_switch
.global initialize_task_stack
.global select_next_task
.global invoke_first_task
.global return_to_main
.global g_p_current_task
.global main_stack

/*
This function restores the main stack and resumes executing
*/
return_to_main:
  /* restore 'main' sp */
  la t0, main_stack
  lw sp, 0(t0)
  /* restore 'main' state */
  M_PSP_POP
  /* resume 'main' execution */
  ret

/*
Entry point to trigger the first task
a0 - address of taskCB_t to start
*/
invoke_first_task:
  /* save the 'main' state */
  M_PSP_PUSH
  /* save the 'main' sp */
  la t0, main_stack
  sw sp, 0(t0)
  /* prepare argument for select_next_task - currently no task */
  mv  a0, zero
  j context_switch_first_task

/*
Change running task by switching stack address
*/
context_switch:
  /* save current task registers */
  M_PSP_PUSH
  /* prepare argument for select_next_task - current sp address */
  mv  a0, sp
context_switch_first_task:
  /* select the next task to execute */
  jal select_next_task
  /* we got now a new stack address - update the sp value */
  mv  sp, a0
  /* restore registers of the selected task */
  M_PSP_POP
  /* continue executing the newly selected task */
  ret

/*
Initialize the task stack with ra address
a0 - task handler address
a1 - stack address
return - new stack address
*/
initialize_task_stack:
  /* save the return address */
  sw   a0, 0(a1)
  /* return new stack address */
  addi a0, a1, 4
  addi a0, a0, -FRAME_SIZE
  ret
