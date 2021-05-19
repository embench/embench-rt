#ifndef __CONTEXT_SWITCH_LATENCY_PORT_RV_H__
#define __CONTEXT_SWITCH_LATENCY_PORT_RV_H__

#ifdef D_RISCV
    #ifdef D_CYCLES
       #define M_READ_CYCLE_COUNTER(var)     asm volatile ("csrr %0, mcycle" : "=r"(var));
       #define M_READ_CYCLE_COUNTER_END(var) M_READ_CYCLE_COUNTER(var)
    #else
       #define M_READ_CYCLE_COUNTER(var)     asm volatile ("csrr %0, minstret" : "=r"(var));
       #define M_READ_CYCLE_COUNTER_END(var) M_READ_CYCLE_COUNTER(var)
    #endif /* D_CYCLES */
#else
    #ifdef D_CYCLES
       #define M_READ_CYCLE_COUNTER(var)
       #define M_READ_CYCLE_COUNTER_END(var)
    #else
       #define M_READ_CYCLE_COUNTER(var)     
       #define M_READ_CYCLE_COUNTER_END(var)
    #endif /* D_CYCLES */
#endif /* D_RISCV */

#endif /* __CONTEXT_SWITCH_LATENCY_PORT_RV_H__ */
