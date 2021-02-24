#include "int-latency.h"

#define D_MSTATUS_MIE_MASK     0x00000008
#define D_MIE_MEIE_MASK        0x00000800

#define _WRITE_CSR_(reg, val) ({ \
  if (__builtin_constant_p(val) && (unsigned long)(val) < 32) \
    asm volatile ("csrw " #reg ", %0" :: "i"(val)); \
  else \
    asm volatile ("csrw " #reg ", %0" :: "r"(val)); })
#define _WRITE_CSR_INTERMEDIATE_(reg, val) _WRITE_CSR_(reg, val)
/* write an entire CSR */
#define M_WRITE_CSR(csr, val)   _WRITE_CSR_INTERMEDIATE_(csr, val)

/* set specific bits in a CSR */
#define M_CLEAR_CSR_BITS(reg, bits) ({\
  if (__builtin_constant_p(bits) && (unsigned long)(bits) < 32) \
    asm volatile ("csrc " #reg ", %0" :: "i"(bits)); \
  else \
    asm volatile ("csrc " #reg ", %0" :: "r"(bits)); })

/* clear specific bits in a CSR */
#define M_SET_CSR_BITS(reg, bits) ({\
    if (__builtin_constant_p(bits) && (unsigned long)(bits) < 32) \
      asm volatile ("csrs " #reg ", %0" :: "i"(bits)); \
    else \
      asm volatile ("csrs " #reg ", %0" :: "r"(bits)); })

/* fence instruction */
#define M_FENCE() asm volatile("fence")

/* 
*   enable external interrupts
*/
void bsp_enble_external_interrupt(void)
{
  /* TODO: write here the code that enables the external interrupt */
  
  /* enable external interrupts in mie scr */
  M_SET_CSR_BITS(mie, D_MIE_MEIE_MASK);
}

/* 
*   Trigger the external interrupt
*/
void bsp_trigger_external_interrupt(void)
{
  /* TODO: write here the code that triggers the external interrupt */
  
  /* TODO: uncomment the following line if fence instruction is required */
  /* M_FENCE(); */
}

volatile cycles_t cycles;

/* 
*   This function is responsible sampling cpu cycles for 'triggering
*   an external interrupt' operation; it will trigger the interrupt
*   and sample the current value of the cpu cycles. The measure start
*   point is prior to calling this function so that we'll get the cost
*   in cycles of 'triggering external interrupt' operation
* 
*   p_cycles - value of sampled cpu cycles 
*/
void bsp_trigger_external_interrupt_sample_cycles(volatile cycles_t* p_cycles)
{
  /* TODO: write here the code that triggers the external interrupt */
  
  /* read the value of mcycles register */
  M_READ_CYCLE_COUNTER_END(cycles);
  
  /* TODO: uncomment the following line if fence instruction is required */
  /* M_FENCE(); */

  /* save the measured cycles */
  *p_cycles = cycles;
}

/* 
*   Clear the external interrupt indication 
*/
void bsp_clear_external_interrupt_indication(void)
{
  /* TODO: write here the code to clear the external interrupt indication */

}

/* 
*   Register a trap handler or vector table
* 
*   p_ints_handler - address of trap handler or vector table 
*   is_vector      - 0, p_ints_handler is a trap handler
*                    1, p_ints_handler is vector table
*                    This value may be redandent in none riscv cores
*/
void bsp_set_interrupts_handler(void *p_ints_handler, unsigned int is_vector)
{
  unsigned int ints_handler;

  /* is_vector can be 0 or 1 only */
  if (is_vector == 0 || is_vector == 1)
  {
    /* prepare the calue of mtvec */
    ints_handler = ((unsigned int)p_ints_handler) | is_vector;
    /* write the value of mtvec */
    M_WRITE_CSR(mtvec, ints_handler);
  }
}

/* 
*   global enable interrupts
*/
void bsp_enable_interrupts(void)
{
  /* set mie bit in mstatus */
  M_SET_CSR_BITS(mstatus, D_MSTATUS_MIE_MASK);
}

/* 
*   global disable interrupts
*/
void bsp_disable_interrupts(void)
{
  /* clear mie bit in mstatus */
  M_CLEAR_CSR_BITS(mstatus, D_MSTATUS_MIE_MASK);
}

/* 
*   bsp specific initialization
*/
void bsp_init(void)
{
  /* TODO: add here any bsp init */
}
