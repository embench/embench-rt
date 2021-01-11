# RiscV interrupt latency benchmark

## Interrupt Latency Measurement Concept

### Assumption
The provided core is connected to at least one  external interrupt source that can be triggered by a running firmware.

### Measurement

Interrupt latency measurement is done by using the cycles counter (`mcycle` and `mcycleh`).
1. Measure from interrupt trigger to vector/trap entry
Start measurement point: when interrupts are enabled (`mstatus`) and the configured external interrupt enabled.
End measurement point depends on `mtvec` mode:
- Trap mode: first 2 instructions read `mcycle` and `mcycleh` counters
- Vector mode: first 2 instructions read `mcycle` and `mcycleh` counters
2. Measure from interrupt trigger to isr (in both vector/trap mode)
Start measurement point: when interrupts are enabled (`mstatus`) and the configured external interrupt enabled.
End measurement point: isr entry

Read `mcycle` and `mcycleh` counters are done to core registers `t5` and `t6` (it is assumed they are not used in the said flow)

### Benchmark flow
1. Measure the amount of cycles cost of overhead code (how much does it cost to measure)
2. Configure and enable a specific external interrupt - this external interrupt source is used to perform the latency measurements.
3. Enable external interrupts
4. measure cycles from interrupt trigger to vector entry
5. measure cycles from interrupt trigger to trap entry
6. measure cycles from interrupt trigger to isr entry (vector mode)
7. measure cycles from interrupt trigger to isr entry (trap mode)

## BSP

The following functions implementation shall be provided per BSP:

- void bsp_init(void) -> BSP specific initialization function.
- void bsp_enble_external_interrupt(void) -> Set and enable a specific external interrupt
- void bsp_trigger_external_interrupt(void) -> Trigger the configured external interrupt
- void bsp_clear_external_interrupt_indication(void) -> Clear the external interrupt indication - called from the interrupt handler
- extern void bsp_enable_interrupts(void) -> system disable interrupts
- extern void bsp_disable_interrupts(void) -> system enable interrupts
- extern void bsp_trigger_external_interrupt_measure_cycles(volatile cycles_t* p_cycles) -> measure the cost in cycles of trigegrring the external interrupt   
- extern void bsp_set_interrupts_handler(void *p_ints_handler, unsigned int is_vector) -> set interrupt vector/trap address

## Benchmark main function
```
static int __attribute__ ((noinline))
benchmark_body (int rpt)
{
  /* initialize and enable a specific external interrupt */
  bsp_enble_external_interrupt();

  /* disable interrupts */
  bsp_disable_interrupts();

  /* measure cycles overhead */
  if (measure_overhead_cycles_trigger_ext_int(rpt))
  {
    /* fail to measure cycles overhead */
    return 1;
  }

  /* enable interrupts */
  bsp_enable_interrupts();

  /*
   * measure from interrupt trigger to trap/vector entry
   */
  /* initialize interrupt counter - how many interrupts occurred */
  g_vect_count = 0;

  /* measure number of cycles from interrupt to vector start */
  cycles_to_vect_entry = measure_int_latency(rpt, &g_vect_count, (void*)psp_vect_table,
                                 1, &g_num_of_cycles);

  /* initialize interrupt counter - how many interrupts occurred */
  g_trap_count = 0;

  /* measure number of cycles from interrupt to trap start */
  cycles_to_trap_entry = measure_int_latency(rpt, &g_trap_count, (void*)psp_trap_handler,
                                  0, &g_num_of_cycles);

  /*
   * measure from interrupt trigger to isr entry
   */

  /* initialize interrupt counter - how many interrupts occurred */
  g_vect_count = 0;

  /* measure number of cycles from interrupt to isr via vector */
  cycles_to_isr_vect_mode = measure_int_latency(rpt, &g_vect_count, (void*)psp_vect_table_pure,
                                  1, &g_num_of_cycles_isr_entry);

  /* initialize interrupt counter - how many interrupts occurred */
  g_trap_count = 0;

  /* measure number of cycles from interrupt to isr via trap */
  cycles_to_isr_trap_mode = measure_int_latency(rpt, &g_trap_count, (void*)psp_trap_handler_pure,
                                  0, &g_num_of_cycles_isr_entry);

  return 0;
}
```

## Measurement function
```
unsigned int
measure_int_latency(int rpt, unsigned int* p_int_count, void* p_ints_handler,
                    unsigned int is_vector, volatile cycles_t* p_measure_end)
{
    int loop_count;

    /* set interrupt trap/vector */
    bsp_set_interrupts_handler(p_ints_handler, is_vector);

    /*
     * measure number of cpu cycles in trap/vector mode
     */
    for (loop_count = 0 ; loop_count < rpt ; loop_count++)
    {
        /* read mcycle */
        M_READ_CYCLE_COUNTER_CSR(g_num_of_cycles_start);
        /* trigger a specific external interrupt */
        bsp_trigger_external_interrupt();
        /* number of cycles in trap mode */
        *p_measure_end -= (g_num_of_cycles_start + g_cycles_overhead);
    }

    if (*p_int_count == rpt)
    {
        return *p_measure_end;
    }

    return 0;
}
```
