# Copyright(C) 2019 Hex Five Security, Inc.
# 10-MAR-2019 Cesare Garlati

#include "platform.h"
#include "macro.s"

#define THREADS  8
#define TICK    10
#define COUNT 1000

.section .data
.equ ctx_size, 32*4 # 32 regs x 4 bytes
 ctx_base: .space ctx_size*THREADS; # 32 regs x 4 bytes x 8 threads = 1024 bytes

.section .text
.global start

# -----------------------------------------------------------------------------
start:
# -----------------------------------------------------------------------------

		# perf counters
		# mhpm3: ctx switch counter
		# mhpm4: ctx switch tot cycles
		li a0, COUNT; csrw mhpmcounter3, a0
		csrw mhpmcounter4, zero

		# set trap vector
		la a0, timer; csrw mtvec, a0

		# set timer (10 ms)
		TMR_SET TICK

		# enable timer interrupt
		li a0, 1 << 7; csrw mie, a0
		li a0, 1 << 3; csrw mstatus, a0

		# initialize threads
		la a0, ctx_base;
		la a1, thread0; sw a1, 0*ctx_size(a0)
		la a1, thread1; sw a1, 1*ctx_size(a0)
		la a1, thread2; sw a1, 2*ctx_size(a0)
		la a1, thread3; sw a1, 3*ctx_size(a0)
		la a1, thread4; sw a1, 4*ctx_size(a0)
		la a1, thread5; sw a1, 5*ctx_size(a0)
		la a1, thread6; sw a1, 6*ctx_size(a0)
		la a1, thread7; sw a1, 7*ctx_size(a0)

		# start 1st thread
		la a0, ctx_base; csrw mscratch, a0
		CTX_CLEAR
		j thread0


		.align 8;
# -----------------------------------------------------------------------------
timer:
# -----------------------------------------------------------------------------

		# stats minstret / mcycle
		csrw minstret, zero

		CTX_STORE

		TMR_SET 10

		# next thread ptr
		csrr a0, mscratch
		la a1, ctx_base + THREADS*ctx_size
		addi a0, a0, ctx_size
		bltu a0, a1, 1f
		la a0, ctx_base
1:		csrw mscratch, a0

		CTX_LOAD

		# stats minstret / mcycle
		csrrw t1, minstret, t1
		csrrw t0, mhpmcounter4, t0;	add t0, t1, t0;	csrrw t0, mhpmcounter4, t0
		csrrw t1, minstret, t1

		# count
		csrrw t0, mhpmcounter3, t0
		addi t0, t0, -1; beqz t0, exit
		csrrw t0, mhpmcounter3, t0

		mret

exit:	ebreak


# -----------------------------------------------------------------------------
# threads
# -----------------------------------------------------------------------------

.align 12; thread0: THREAD 0
.align 12; thread1: THREAD 1
.align 12; thread2: THREAD 2
.align 12; thread3: THREAD 3
.align 12; thread4: THREAD 4
.align 12; thread5: THREAD 5
.align 12; thread6: THREAD 6
.align 12; thread7: THREAD 7
