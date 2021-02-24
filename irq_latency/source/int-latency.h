#ifndef __INT_LATENCY_H__
#define __INT_LATENCY_H__

#ifdef D_RISCV
   #ifdef D_64_BIT_CYCLES
       typedef unsigned long long cycles_t;
       #ifdef D_CYCLES
          #define M_READ_CYCLE_COUNTER(var)     asm volatile ("la t4, " #var : ); \
                                                asm volatile ("csrr t5, mcycleh" : ); \
                                                asm volatile ("csrr t6, mcycle" :); \
                                                asm volatile ("sw t6, 0(t4)"); \
                                                asm volatile ("sw t5, 4(t4)");
          #define M_READ_CYCLE_COUNTER_END(var) asm volatile ("csrr t6, mcycle" : ); \
                                                asm volatile ("csrr t5, mcycleh" :); \
                                                asm volatile ("la t4, " #var : ); \
                                                asm volatile ("sw t6, 0(t4)"); \
                                                asm volatile ("sw t5, 4(t4)");
       #else
          #define M_READ_CYCLE_COUNTER(var)     asm volatile ("la t4, " #var : ); \
                                                asm volatile ("csrr t5, minstreth" :); \
                                                asm volatile ("csrr t6, minstret" : ); \
                                                asm volatile ("sw t6, 0(t4)"); \
                                                asm volatile ("sw t5, 4(t4)");
          #define M_READ_CYCLE_COUNTER_END(var) asm volatile ("csrr t6, minstret" : ); \
                                                asm volatile ("csrr t5, minstreth" :); \
                                                asm volatile ("la t4, " #var : ); \
                                                asm volatile ("sw t6, 0(t4)"); \
                                                asm volatile ("sw t5, 4(t4)");
       #endif /* D_CYCLES */
   #else /* D_64_BIT_CYCLES */
       typedef unsigned int cycles_t;
       #ifdef D_CYCLES
          #define M_READ_CYCLE_COUNTER(var)     asm volatile ("csrr %0, mcycle" : "=r"(var));
          #define M_READ_CYCLE_COUNTER_END(var) M_READ_CYCLE_COUNTER(var)
       #else
          #define M_READ_CYCLE_COUNTER(var)     asm volatile ("csrr %0, minstret" : "=r"(var));
          #define M_READ_CYCLE_COUNTER_END(var) M_READ_CYCLE_COUNTER(var)
       #endif /* D_CYCLES */
   #endif /* D_64_BIT_CYCLES */
#else
   #ifdef D_64_BIT_CYCLES
       typedef unsigned int cycles_t;
       #ifdef D_CYCLES
          #define M_READ_CYCLE_COUNTER(var)
          #define M_READ_CYCLE_COUNTER_END(var)
       #else
          #define M_READ_CYCLE_COUNTER(var)     
          #define M_READ_CYCLE_COUNTER_END(var)
       #endif /* D_CYCLES */
   #else /* D_64_BIT_CYCLES */
       typedef unsigned int cycles_t;
       #ifdef D_CYCLES
          #define M_READ_CYCLE_COUNTER(var)
          #define M_READ_CYCLE_COUNTER_END(var)
       #else
          #define M_READ_CYCLE_COUNTER(var)     
          #define M_READ_CYCLE_COUNTER_END(var)
       #endif /* D_CYCLES */
   #endif /* D_64_BIT_CYCLES */
#endif /* D_RISCV */
#endif /* __INT_LATENCY_H__ */
