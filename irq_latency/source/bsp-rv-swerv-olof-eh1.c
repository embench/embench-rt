#include "int-latency.h"

#define D_EXT_INT_IRQ3         3
#define D_PIC_MEIPL_ADDR       0xF00C0000
#define D_PIC_MEIE_ADDR        0xF00C2000
#define D_PIC_MPICCFG_ADDR     0xF00C3000
#define D_PIC_MEIGWCTRL_ADDR   0xF00C4000
#define D_PIC_MEIGWCLR_ADDR    0xF00C5000
#define D_TRIGGER_EXT_INT_ADDR 0x8000100B
#define D_CSR_MEIPT            0xBC9
#define D_CSR_MEICIDPL         0xBCB
#define D_CSR_MEICURPL         0xBCC
#define M_WRITE_REGISTER_32(reg, value)  ((*(volatile unsigned int *)(void*)(reg)) = (value))
#define M_WRITE_REGISTER_08(reg, value)  ((*(volatile unsigned char *)(void*)(reg)) = (value))
#define D_MSTATUS_MIE_MASK     0x00000008
#define D_MIE_MEIE_MASK        0x00000800

#define _WRITE_CSR_(reg, val) ({ \
  if (__builtin_constant_p(val) && (unsigned long)(val) < 32) \
    asm volatile ("csrw " #reg ", %0" :: "i"(val)); \
  else \
    asm volatile ("csrw " #reg ", %0" :: "r"(val)); })
#define _WRITE_CSR_INTERMEDIATE_(reg, val) _WRITE_CSR_(reg, val)
#define M_WRITE_CSR(csr, val)   _WRITE_CSR_INTERMEDIATE_(csr, val)

#define M_CLEAR_CSR_BITS(reg, bits) ({\
  if (__builtin_constant_p(bits) && (unsigned long)(bits) < 32) \
    asm volatile ("csrc " #reg ", %0" :: "i"(bits)); \
  else \
    asm volatile ("csrc " #reg ", %0" :: "r"(bits)); })

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
  /* enable the external interrupt */
  M_WRITE_REGISTER_32(D_PIC_MPICCFG_ADDR, 0);
  M_WRITE_CSR(D_CSR_MEIPT, 0);
  M_WRITE_CSR(D_CSR_MEICIDPL, 0);
  M_WRITE_CSR(D_CSR_MEICURPL, 0);
  M_WRITE_REGISTER_32(D_PIC_MEIGWCTRL_ADDR + (D_EXT_INT_IRQ3*4), 0);
  M_WRITE_REGISTER_32(D_PIC_MEIGWCLR_ADDR + (D_EXT_INT_IRQ3*4), 0);
  M_WRITE_REGISTER_32(D_PIC_MEIPL_ADDR + (D_EXT_INT_IRQ3*4), 15);
  M_WRITE_REGISTER_32(D_PIC_MEIE_ADDR + (D_EXT_INT_IRQ3*4), 1);
  /* enable external interrupts in mie csr */
  M_SET_CSR_BITS(mie, D_MIE_MEIE_MASK);
}

/* 
*   Trigger the external interrupt
*/
void bsp_trigger_external_interrupt(void)
{
  /* trigger the external interrupt */
  M_WRITE_REGISTER_08(D_TRIGGER_EXT_INT_ADDR, (1 << D_EXT_INT_IRQ3));
  M_FENCE();
}

volatile cycles_t cycles;

/* 
*   This function is responsible for ampling cpu cycles for 'triggering
*   an external interrupt' operation; it will trigger the interrupt
*   and sample the current value of the cpu cycles. The measure start
*   point is prior to calling this function so that we'll get the cost
*   in cycles of 'triggering external interrupt' operation
* 
*   p_cycles - value of sampled cpu cycles 
*/
void bsp_trigger_external_interrupt_sample_cycles(volatile cycles_t* p_cycles)
{
  /* triggers the external interrupt */
  M_WRITE_REGISTER_08(D_TRIGGER_EXT_INT_ADDR, (1 << D_EXT_INT_IRQ3));
  /* read the value of mcycles register */
  M_READ_CYCLE_COUNTER_END(cycles);
  M_FENCE();

  *p_cycles = cycles + 9;
}

/* 
*   Clear the external interrupt indication 
*/
void bsp_clear_external_interrupt_indication(void)
{
  /* clear the external interrupt indication */
  M_WRITE_REGISTER_32(D_TRIGGER_EXT_INT_ADDR, 0);
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
