#ifndef __INT_LATENCY_BSP_H__
#define __INT_LATENCY_BSP_H__

/* 
*   bsp specific initialization
*/
void bsp_init(void);

/* 
*   global enable interrupts
*/
void bsp_enable_interrupts(void);

/* 
*   global disanle interrupts
*/
void bsp_disable_interrupts(void);

/* 
*   enable external interrupts
*/
void bsp_enble_external_interrupt(void);

/* 
*   Trigger the external interrupt
*/
void bsp_trigger_external_interrupt(void);

/* 
*   Clear the external interrupt indication 
*/
void bsp_clear_external_interrupt_indication(void);

/* 
*   Register a trap handler or vector table
* 
*   p_ints_handler - address of trap handler or vector table 
*   is_vector      - 0, p_ints_handler is a trap handler
*                    1, p_ints_handler is vector table
*                    This value may be redandent in none riscv cores
*/
void bsp_set_interrupts_handler(void *p_ints_handler, unsigned int is_vector);

/* 
*   This function is responsible sampling cpu cycles for 'triggering
*   an external interrupt' operation; it will trigger the interrupt
*   and sample the current value of the cpu cycles. The measure start
*   point is prior to calling this function so that we'll get the cost
*   in cycles of 'triggering external interrupt' operation
* 
*   p_cycles - value of sampled cpu cycles 
*/
void bsp_trigger_external_interrupt_sample_cycles(volatile cycles_t* p_cycles);

#endif /* __INT_LATENCY_BSP_H__ */
