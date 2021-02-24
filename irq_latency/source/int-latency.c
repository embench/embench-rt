
#include "int-latency.h"
#include "int-latency-bsp.h"

/* local prototypes */
void psp_vect_table(void);
void psp_trap_handler(void);
void psp_vect_table_pure(void);
void psp_trap_handler_pure(void);

/* global variables */
volatile unsigned int cycles_to_vect_entry = 0, cycles_to_trap_entry = 0;
volatile unsigned int cycles_to_isr_vect_mode = 0, cycles_to_isr_trap_mode = 0;
volatile cycles_t g_cycles_overhead;
volatile cycles_t g_num_of_cycles_isr_entry, g_num_of_cycles;
static volatile cycles_t g_num_of_cycles_start;
unsigned int g_trap_count, g_vect_count;

#define D_LOOP_COUNT           256

__attribute__ ((interrupt))
void
interrupt_handler_from_vect(void)
{
  /* read cpu cycle */
  M_READ_CYCLE_COUNTER(g_num_of_cycles_isr_entry);
  /* count number of interrupts in vector mode */
  g_vect_count++;
  /* clear interrupt indication */
  bsp_clear_external_interrupt_indication();
}

void
interrupt_handler_from_trap(void)
{
  /* read cpu cycle */
  M_READ_CYCLE_COUNTER(g_num_of_cycles_isr_entry);
  /* count number of interrupts in trap mode */
  g_trap_count++;
  /* clear interrupt indication */
  bsp_clear_external_interrupt_indication();
}

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
        /* read cpu cycle */
        M_READ_CYCLE_COUNTER(g_num_of_cycles_start);
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

unsigned int
measure_overhead_cycles_trigger_ext_int(int rpt)
{
    int loop_count;

    /* make the code worm - measure cycles penalty */
    for (loop_count = 0 ; loop_count < rpt ; loop_count++)
    {
        /* read cpu cycle */
        M_READ_CYCLE_COUNTER(g_num_of_cycles_start);
        /* we want to measure number of cycles for triggering the interrupt */
        bsp_trigger_external_interrupt_sample_cycles(&g_cycles_overhead);
        /* make sure measurement is OK */
        if (g_cycles_overhead <= g_num_of_cycles_start)
        {
            return 1;
        }
        /* calculate cycles overhead */
        g_cycles_overhead -= g_num_of_cycles_start;
        /* clear interrupt indication */
        bsp_clear_external_interrupt_indication();
    }

    return 0;
}

int
verify_benchmark (int res __attribute ((unused)))
{
  return 0;
}


void
initialise_benchmark (void)
{
}


static int benchmark_body (int  rpt);

void
warm_caches (int  heat)
{
  benchmark_body (heat);

  return;
}

int
benchmark (void)
{
  return benchmark_body (D_LOOP_COUNT);
}

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

#ifdef D_CORE_HAS_TRAP
  /* initialize interrupt counter - how many interrupts occurred */
  g_trap_count = 0;

  /* measure number of cycles from interrupt to trap start */
  cycles_to_trap_entry = measure_int_latency(rpt, &g_trap_count, (void*)psp_trap_handler,
                                  0, &g_num_of_cycles);
#endif /* D_CORE_HAS_TRAP */

  /*
   * measure from interrupt trigger to isr entry
   */

  /* initialize interrupt counter - how many interrupts occurred */
  g_vect_count = 0;

  /* measure number of cycles from interrupt to isr via vector */
  cycles_to_isr_vect_mode = measure_int_latency(rpt, &g_vect_count, (void*)psp_vect_table_pure,
                                  1, &g_num_of_cycles_isr_entry);

#ifdef D_CORE_HAS_TRAP
  /* initialize interrupt counter - how many interrupts occurred */
  g_trap_count = 0;

  /* measure number of cycles from interrupt to isr via trap */
  cycles_to_isr_trap_mode = measure_int_latency(rpt, &g_trap_count, (void*)psp_trap_handler_pure,
                                  0, &g_num_of_cycles_isr_entry);
#endif /* D_CORE_HAS_TRAP */

  return 0;
}

/*
   Local Variables:
   mode: C
   c-file-style: "gnu"
   End:
*/
