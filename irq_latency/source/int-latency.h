#ifndef __INT_LATENCY_H__
#define __INT_LATENCY_H__

#ifdef D_64_BIT_CYCLES
    typedef unsigned long long cycles_t;
    #ifdef D_CYCLES
    #define M_READ_CYCLE_COUNTER_CSR(var) asm volatile ("la t4, " #var : ); \
                                          asm volatile ("csrr t5, mcycleh" : ); \
                                          asm volatile ("csrr t6, mcycle" :); \
                                          asm volatile ("sw t6, 0(t4)"); \
                                          asm volatile ("sw t5, 4(t4)");
    #define M_READ_CYCLE_COUNTER_CSR_END(var) asm volatile ("csrr t6, mcycle" : ); \
                                          asm volatile ("csrr t5, mcycleh" :); \
                                          asm volatile ("la t4, " #var : ); \
                                          asm volatile ("sw t6, 0(t4)"); \
                                          asm volatile ("sw t5, 4(t4)");
    #else
    #define M_READ_CYCLE_COUNTER_CSR(var) asm volatile ("la t4, " #var : ); \
                                          asm volatile ("csrr t5, minstreth" :); \
                                          asm volatile ("csrr t6, minstret" : ); \
                                          asm volatile ("sw t6, 0(t4)"); \
                                          asm volatile ("sw t5, 4(t4)");
    #define M_READ_CYCLE_COUNTER_CSR_END(var) asm volatile ("csrr t6, minstret" : ); \
                                          asm volatile ("csrr t5, minstreth" :); \
                                          asm volatile ("la t4, " #var : ); \
                                          asm volatile ("sw t6, 0(t4)"); \
                                          asm volatile ("sw t5, 4(t4)");
    #endif
#else
    typedef unsigned int cycles_t;
    #ifdef D_CYCLES
    #define M_READ_CYCLE_COUNTER_CSR(var) asm volatile ("csrr %0, mcycle" : "=r"(var));
    #else
    #define M_READ_CYCLE_COUNTER_CSR(var) asm volatile ("csrr %0, minstret" : "=r"(var));
    #endif
#endif

#endif /* __INT_LATENCY_H__ */
