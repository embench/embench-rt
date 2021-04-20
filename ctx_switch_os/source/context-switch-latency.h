#ifndef __CONTEXT_SWITCH_LATENCY_H__
#define __CONTEXT_SWITCH_LATENCY_H__

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

/* functions implemented int context-switch-latency-rv.S */
void return_to_main(void);
void context_switch(void);
void invoke_first_task(void* p_task_stack);
void* initialize_task_stack(void* p_func_handler, void* p_stack_address);

#endif /* __CONTEXT_SWITCH_LATENCY_H__ */
