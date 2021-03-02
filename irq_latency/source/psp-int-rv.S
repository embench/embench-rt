.equ REGBYTES, 4
.macro M_PSP_PUSH
  addi    sp,sp,-64
  sw  ra,60(sp)
  sw  t0,56(sp)
  sw  t1,52(sp)
  sw  t2,48(sp)
  sw  a0,44(sp)
  sw  a1,40(sp)
  sw  a2,36(sp)
  sw  a3,32(sp)
  sw  a4,28(sp)
  sw  a5,24(sp)
  sw  a6,20(sp)
  sw  a7,16(sp)
  sw  t3,12(sp)
  sw  t4,8(sp)
  sw  t5,4(sp)
  sw  t6,0(sp)
.endm

.macro M_PSP_POP
 lw  ra,60(sp)
 lw  t0,56(sp)
 lw  t1,52(sp)
 lw  t2,48(sp)
 lw  a0,44(sp)
 lw  a1,40(sp)
 lw  a2,36(sp)
 lw  a3,32(sp)
 lw  a4,28(sp)
 lw  a5,24(sp)
 lw  a6,20(sp)
 lw  a7,16(sp)
 lw  t3,12(sp)
 lw  t4,8(sp)
 lw  t5,4(sp)
 lw  t6,0(sp)
 addi    sp,sp,64
.endm

#ifdef D_64_BIT_CYCLES
    #ifdef D_CYCLES
        .macro M_READ_CYCLES var
            csrr    t6,mcycle
            csrr    t5,mcycleh
            la      t4, \var
            sw      t6,0(t4)
            sw      t5,4(t4)
        .endm
    #else
        .macro M_READ_CYCLES var
            csrr    t6,minstret
            csrr    t5,minstreth
            la      t4, \var
            sw      t6,0(t4)
            sw      t5,4(t4)
        .endm
    #endif
    #else
    #ifdef D_CYCLES
        .macro M_READ_CYCLES var
            csrr    t6,mcycle
            la      t5, \var
            sw      t6,0(t5)
        .endm
    #else
        .macro M_READ_CYCLES var
            csrr    t6,minstret
            la      t5, \var
            sw      t6,0(t5)
        .endm
    #endif
#endif
.section  .text
.global psp_vect_table
.global psp_trap_handler
.global psp_vect_table_pure
.global psp_trap_handler_pure
.extern g_num_of_cycles

.align 4
psp_trap_handler:
    /* read mcycle csr */
    M_READ_CYCLES g_num_of_cycles
    /* save regs */
    M_PSP_PUSH
    csrr    t0, mcause
    li      t1, 11
    and     t0, t0, t1
    bne     t0, t1, psp_reserved_int
    /* call external interrupt handler */
    jal     interrupt_handler_from_trap
    /* restore regs */
    M_PSP_POP
    mret

.align 4
psp_trap_handler_pure:
    /* save regs */
    M_PSP_PUSH
    csrr    t0, mcause
    li      t1, 11
    and     t0, t0, t1
    bne     t0, t1, psp_reserved_int
    /* call external interrupt handler */
    jal     interrupt_handler_from_trap
    /* restore regs */
    M_PSP_POP
    mret

.align 4
psp_vect_table:
    j psp_reserved_int
    .align 2
    j psp_reserved_int
    .align 2
    j psp_reserved_int
    .align 2
    j psp_reserved_int
    .align 2
    j psp_reserved_int
    .align 2
    j psp_reserved_int
    .align 2
    j psp_reserved_int
    .align 2
    j psp_reserved_int
    .align 2
    j psp_reserved_int
    .align 2
    j psp_reserved_int
    .align 2
    j psp_reserved_int
    .align 2
    M_READ_CYCLES g_num_of_cycles
    /* call external interrupt handler */
    j interrupt_handler_from_vect

.align 4
psp_vect_table_pure:
    j psp_reserved_int
    .align 2
    j psp_reserved_int
    .align 2
    j psp_reserved_int
    .align 2
    j psp_reserved_int
    .align 2
    j psp_reserved_int
    .align 2
    j psp_reserved_int
    .align 2
    j psp_reserved_int
    .align 2
    j psp_reserved_int
    .align 2
    j psp_reserved_int
    .align 2
    j psp_reserved_int
    .align 2
    j psp_reserved_int
    .align 2
    /* call external interrupt handler */
    j interrupt_handler_from_vect

psp_reserved_int:
1:
    nop
    nop
    j 1b
