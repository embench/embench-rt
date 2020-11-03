# RiscV interrupt latency benchmark
## BSP
The following functions implementation shall be provided per BSP:
- void bsp_init(void) -> BSP specific initialization function.
- void bsp_enble_external_interrupt(void) -> Set and enable a specific external interrupt
- void bsp_trigger_external_interrupt(void) -> Trigger the configured external interrupt
- void bsp_clear_external_interrupt_indication(void) -> Clear the external interrupt indication - called from the interrupt handler

## Benchmark main function
```
static int __attribute__ ((noinline))
benchmark_body (int rpt)
{
  unsigned int loop_count, loc_vec_cnt = 0, loc_trp_cnt = 0;

  /* read mcycle and mcycleh */
  M_READ_CYCLE_COUNTER_CSR(g_num_of_cycles_start);
  /* we want to measure number of cycles for write csr and read csr */
  M_SET_CSR_BITS(mstatus, D_MSTATUS_MIE_MASK);
  M_READ_CYCLE_COUNTER_CSR(tmp);
  /* read mcycle */
  M_READ_CYCLE_COUNTER_CSR(cycles_overhead);
  /* calculate cycles overhead */
  cycles_overhead -= g_num_of_cycles_start;

  /* initialize and enable a specific external interrupt */
  bsp_enble_external_interrupt();

  for (loop_count = 0 ; loop_count < D_LOOP_COUNT ; loop_count++)
  {
      /*
       * measure number of cpu cycles in vector mode
       */
      /* disable interrupts */
      M_CLEAR_CSR_BITS(mstatus, D_MSTATUS_MIE_MASK);
      /* enable external interrupts in mie */
      M_SET_CSR_BITS(mie, D_MIE_MEIE_MASK);
      /* init mtvec - vector mode */
      M_WRITE_CSR(mtvec, (unsigned int)psp_vect_table | 1);
      /* trigger a specific external interrupt */
      bsp_trigger_external_interrupt();
      /* read mcycle */
      M_READ_CYCLE_COUNTER_CSR(g_num_of_cycles_start);
      /* enable interrupts */
      M_SET_CSR_BITS(mstatus, D_MSTATUS_MIE_MASK);
      /* verify we got the interrupt */
      if (loc_vec_cnt + 1 == vect_count)
      {
          loc_vec_cnt++;
          /* number of cycles in vector mode */
          g_num_of_cycles_vect -= (g_num_of_cycles_start /* + cycles_overhead*/);
          /* total cycles in vector mode */
          vect_total += g_num_of_cycles_vect;
      }

      /*
       * measure number of cpu cycles in trap mode
       */
      /* disable interrupts */
      M_CLEAR_CSR_BITS(mstatus, D_MSTATUS_MIE_MASK);
      /* init mtvec - trap mode */
      M_WRITE_CSR(mtvec, psp_trap_handler);
      /* trigger a specific external interrupt */
      bsp_trigger_external_interrupt();
      /* read mcycle */
      M_READ_CYCLE_COUNTER_CSR(g_num_of_cycles_start);
      /* enable interrupts */
      M_SET_CSR_BITS(mstatus, D_MSTATUS_MIE_MASK);
      /* verify we got the interrupt */
      if (loc_trp_cnt + 1 == trap_count)
      {
          loc_trp_cnt++;
          /* number of cycles in trap mode */
          g_num_of_cycles_trap -= (g_num_of_cycles_start /* + cycles_overhead*/);
          /* total cycles in trap mode */
          trap_total += g_num_of_cycles_trap;
      }
  }

  if (trap_count == D_LOOP_COUNT && vect_count == D_LOOP_COUNT)
  {
    trap_total /= D_LOOP_COUNT;
    vect_total /= D_LOOP_COUNT;
  }
  else
  {
    trap_total /= loc_trp_cnt;
    vect_total /= loc_vec_cnt;
  }

  return 0;
}

```
## Trap mode
### Trap handler
```
psp_trap_handler:
    /* read mcycle csr */
    csrr    t6,mcycle
    csrr    t5,mcycleh
    la      t4, g_num_of_cycles_trap
    sw      t6,0(t4)
    sw      t5,4(t4)
    /* save regs */
    M_PSP_PUSH
    csrr    t0, mcause
    bge     t0, zero, psp_reserved_int
    li      t1, 11
    and     t0, t0, t1
    bne     t0, t1, psp_reserved_int
    /* call external interrupt handler */
    jal     interrupt_handler_from_trap
    /* restore regs */
    M_PSP_POP
    mret
```
### Interrupt handler
```
void
interrupt_handler_from_trap(void)
{
  /* count number of interrupts in trap mode */
  trap_count++;
  /* clear interrupt indication */
  bsp_clear_external_interrupt_indication();
}
```
## Vector mode 
### Vector table
```
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
    /* read mcycle csr */
    csrr    t6,mcycle
    csrr    t5,mcycleh
    la      t4, g_num_of_cycles_vect
    sw      t6,0(t4)
    sw      t5,4(t4)
    /* call external interrupt handler */
    j interrupt_handler_from_vect
```
### Interrupt handler
```
__attribute__ ((interrupt))
void
interrupt_handler_from_vect(void)
{
  /* count number of interrupts in vector mode */
  vect_count++;
  /* clear interrupt indication */
  bsp_clear_external_interrupt_indication();
}
```
