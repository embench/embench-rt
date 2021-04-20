
#include "context-switch-latency.h"

#define D_LOOP_COUNT     1
#define D_NUM_OF_TASKS   2
#define D_STACK_SIZE     64
#define D_EVENT_BITS     0x51
#define D_AND            0
#define D_OR             0
#define D_WAIT_FOREVER   0
#define D_MAX_QUEUE_SIZE 5

/* task handler function definition */
typedef void (*task_handler)(void);
/* tasks stack */
unsigned int task0_stack[D_STACK_SIZE];
unsigned int task1_stack[D_STACK_SIZE];

/* task control block */
typedef struct taskCB
{
  /* task stack address */
  void         *pStack;
  /* task handler function */
  task_handler  func;
}taskCB_t;

/* semaphore control block */
typedef struct semaphoreCB
{
  /* semaphore counter */
  unsigned int  counter;
  /* semaphore max count */
  unsigned int  max_count;
  /* pending tasks */
  taskCB_t  *pPendingTasks;
}semaphoreCB_t;

/* event control block */
typedef struct eventCB
{
  /* event bits */
  unsigned int  expected_bits;
  /* wait condition */
  unsigned int  expected_bits_state;
  /* pending task */
  taskCB_t  *pPendingTasks;
}eventCB_t;

/* event control block */
typedef struct queueCB
{
  /* queue */
  unsigned int queue[D_MAX_QUEUE_SIZE];
  /* queue push index */
  unsigned int  push_index;
  /* queue pop index */
  unsigned int  pop_index;
  /* queue number of items in the queue */
  unsigned int  num_of_items;
  /* pending task */
  taskCB_t  *pPendingTasks;
}queueCB_t;

/* tasks handlers functions */
static void task0_func(void);
static void task1_func(void);

/* global variables */
taskCB_t *g_p_current_task;
static volatile cycles_t g_num_of_cycles_start;
static volatile cycles_t g_num_of_cycles_event_set_end;
static volatile cycles_t g_num_of_cycles_semaphore_give_end;
static volatile cycles_t g_num_of_cycles_queue_send_end;
static volatile cycles_t g_num_of_cycles_task_yield_end;
static semaphoreCB_t g_sem;
static eventCB_t     g_event;
static queueCB_t     g_queue;

taskCB_t taskReadyList[D_NUM_OF_TASKS] = {
		{task0_stack, task0_func},
		{task1_stack, task1_func},
};

/*
 * Read event bits
 * p_event - semaphore handle
 * get_bits - expected bits
 * bits_condition - can be D_AND or D_OR
 * wait_time - wait timeout (support D_WAIT_FOREVER only as we have no actual timer)
 */
static unsigned int __attribute__ ((noinline))
event_get(eventCB_t *p_event, unsigned int get_bits, unsigned int bits_condition, unsigned int wait_time)
{
  unsigned int bits;
  /* loop until event received as expected */
  while (1)
  {
    /* get the set bits */
    bits = p_event->expected_bits & get_bits;
    /* if all/some bits are set */
    if ((bits_condition == D_AND && bits == get_bits) || ((bits_condition == D_OR && bits)))
    {
      /* do we need to clear the list */
      if (p_event->pPendingTasks == g_p_current_task)
      {
        p_event->pPendingTasks = 0;
      }
      break;
    }
    /* for a zero wait_time, we only switch context w/o
       any real timer */
    else if (wait_time == 0)
    {
      p_event->pPendingTasks = g_p_current_task;
      context_switch();
      /* we completed the event_set */
      M_READ_CYCLE_COUNTER(g_num_of_cycles_event_set_end);
      g_num_of_cycles_event_set_end -= g_num_of_cycles_start;
    }
    else
    {
      return 0;
    }
  }

  return bits;
}

/*
 * Set an event bits
 * p_event - event handle
 * set_bits - bits to set
 */
static void __attribute__ ((noinline))
event_set(eventCB_t *p_event, unsigned int set_bits)
{
  /* set the bits */
  p_event->expected_bits |= set_bits;
  /* check if there are pending tasks */
  if (p_event->pPendingTasks != 0)
  {
    context_switch();
  }
}

/*
 * Acquire semaphore
 * p_sem - semaphore handle
 * wait_time - wait timeout in case semaphore isn't available
 *            (support D_WAIT_FOREVER only as we have no actual timer)
 */
static unsigned int __attribute__ ((noinline))
semaphore_take(semaphoreCB_t* p_sem, unsigned int wait_time)
{
  /* loop until semaphore is available */
  while (1)
  {
    /* is semaphore available */
    if (p_sem->counter && p_sem->counter <= p_sem->max_count)
    {
      /* if task was pending - remove it */
      if (p_sem->pPendingTasks == g_p_current_task)
      {
        p_sem->pPendingTasks = 0;
      }
      /* decrement counter */
      p_sem->counter--;
      /* semaphore is taken */
      return 1;
    }
    /* for a zero wait_time, we only switch context w/o
       any real timer */
    else if (wait_time == D_WAIT_FOREVER)
    {
      p_sem->pPendingTasks = g_p_current_task;
      context_switch();
      /* measure semaphore_give cycles */
      M_READ_CYCLE_COUNTER(g_num_of_cycles_semaphore_give_end);
      g_num_of_cycles_semaphore_give_end -= g_num_of_cycles_start;
    }
    /* no wait time */
    else
    {
      break;
    }
  }
  /* semaphore not available */
  return 0;
}

/*
 * Release a semaphore
 * p_sem - semaphore handle
 */
static unsigned int __attribute__ ((noinline))
semaphore_give(semaphoreCB_t* p_sem)
{
  /* verify semaphore counter */
  if (p_sem->counter < p_sem->max_count)
  {
    /* increment counter */
    p_sem->counter++;
    /* if tasks are pending this semaphore */
    if (p_sem->pPendingTasks != 0)
    {
      context_switch();
    }
    /* semaphore given */
    return 1;
  }
  /* semaphore not given */
  return 0;
}

/*
 * Read an item from a queue
 * p_queue - queue handle
 * p_item - the read item
 * wait_time - wait timeout (support D_WAIT_FOREVER only as we have no actual timer)
 */
static int __attribute__ ((noinline))
queue_receive(queueCB_t* p_queue, unsigned int *p_item, unsigned int wait_time)
{
  /* loop until we get a queue item */
  while (1)
  {
    /* check if the queue is empty */
    if (p_queue->num_of_items == 0)
    {
      /* for a zero wait_time, we only switch context w/o
         any real timer */
      if (wait_time == D_WAIT_FOREVER)
      {
        /* task is pending the queue */
        p_queue->pPendingTasks = g_p_current_task;
        /* switch to other task */
        context_switch();
        /* measure queue_send cycles */
        M_READ_CYCLE_COUNTER(g_num_of_cycles_queue_send_end);
        g_num_of_cycles_queue_send_end -= g_num_of_cycles_start;
      }
      else
      {
        break;
      }
    }
    else
    {
      /* do we need to clear the list */
      if (p_queue->pPendingTasks == g_p_current_task)
      {
        p_queue->pPendingTasks = 0;
      }
      /* read queue item */
      *p_item = p_queue->queue[p_queue->pop_index];
      /* increment pop index */
      p_queue->pop_index = (p_queue->pop_index + 1) % D_MAX_QUEUE_SIZE;
      /* decrement number of items in the queue */
      p_queue->num_of_items--;
      return 1;
    }
  }

  /* fail to get a queue item */
  return 0;
}

/*
 * Write an item to a queue
 * p_queue - queue handle
 * p_item - the item to write
 */
static int __attribute__ ((noinline))
queue_send(queueCB_t* p_queue, unsigned int *p_item)
{
  /* check if the queue is full */
  if (p_queue->num_of_items == D_MAX_QUEUE_SIZE)
  {
    return 0;
  }

  /* write queue item */
  p_queue->queue[p_queue->push_index] = *p_item;
  /* increment push index */
  p_queue->push_index = (p_queue->pop_index + 1) % D_MAX_QUEUE_SIZE;
  /* increment number of items in the queue */
  p_queue->num_of_items++;

  /* if tasks are pending this queue */
  if (p_queue->pPendingTasks != 0)
  {
    context_switch();
    /* measure yield cycles */
    M_READ_CYCLE_COUNTER(g_num_of_cycles_task_yield_end);
    g_num_of_cycles_task_yield_end -= g_num_of_cycles_start;
  }

  return 1;
}

/*
 * Yield CPU time to another task
 */
static void __attribute__ ((noinline))
task_yield(void)
{
  context_switch();
}

/*
 * Task 1 function
 */
void task0_func(void)
{
  unsigned int queue_item = 0;

  /* take a semaphore */
  semaphore_take(&g_sem, D_WAIT_FOREVER);
  /* wait for events */
  event_get(&g_event, D_EVENT_BITS, D_OR, D_WAIT_FOREVER);
  /* receive an item from the queue */
  queue_receive(&g_queue, &queue_item,  D_WAIT_FOREVER);
  /* yield */
  M_READ_CYCLE_COUNTER(g_num_of_cycles_start);
  task_yield();
}

/*
 * Task 1 function
 */
void task1_func(void)
{
  unsigned int queue_item = 0x12345678;
  /* read cpu cycle - start measure semaphore_give */
  M_READ_CYCLE_COUNTER(g_num_of_cycles_start);
  semaphore_give(&g_sem);
  /* read cpu cycle - start measure event_set */
  M_READ_CYCLE_COUNTER(g_num_of_cycles_start);
  event_set(&g_event, D_EVENT_BITS);
  /* read cpu cycle - start measure queue_send */
  M_READ_CYCLE_COUNTER(g_num_of_cycles_start);
  queue_send(&g_queue, &queue_item);
  /* quit the benchmark */
  return_to_main();
}

/*
 * Select the next running task
 * p_task_sp - current task sp
 * return - selected task sp
 */
void* select_next_task(void* p_task_sp)
{
  static unsigned int task_id = 0;

  /* save current task sp */
  taskReadyList[task_id].pStack = p_task_sp;
  /* move to the next task */
  if (++task_id == D_NUM_OF_TASKS)
  {
	/* cyclic queue */
    task_id = 0;
  }

  /* update current running task */
  g_p_current_task = &taskReadyList[task_id];

  /* return sp of the newly selected task */
  return g_p_current_task->pStack;
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

/*
 * initialize semaphore
 */
void init_semaphore(semaphoreCB_t* p_sem)
{
   p_sem->counter = 0;
   p_sem->max_count = 1;
   p_sem->pPendingTasks = 0;
}

/*
 * initialize event
 */
void init_event(eventCB_t* p_event)
{
  p_event->expected_bits = 0;
  p_event->expected_bits_state = 0;
  p_event->pPendingTasks = 0;
}

/*
 * initialize queue
 */
void init_queue(queueCB_t* p_queue)
{
  p_queue->push_index = 0;
  p_queue->pop_index = 0;
  p_queue->num_of_items = 0;
  p_queue->pPendingTasks = 0;
}

static int __attribute__ ((noinline))
benchmark_body (int rpt)
{
  unsigned char i, j;

  for (j = 0 ; j < rpt ; j++)
  {
    /* initialize the stack of each task */
    for (i = 0 ; i < D_NUM_OF_TASKS ; i++)
    {
      taskReadyList[i].pStack = initialize_task_stack(taskReadyList[i].func, taskReadyList[i].pStack + 4*(D_STACK_SIZE - 1));
    }

    init_semaphore(&g_sem);
    init_event(&g_event);
    init_queue(&g_queue);
    invoke_first_task(&taskReadyList[0]);
  }

  return 0;
}

/*
   Local Variables:
   mode: C
   c-file-style: "gnu"
   End:
*/
