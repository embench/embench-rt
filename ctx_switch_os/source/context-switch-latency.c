
#include "context-switch-latency.h"
#ifdef D_RISCV
 #include "context-switch-latency-port-rv.h"
#else 
 #error "missing core definition" 
#endif /* D_RISCV */

#define D_LOOP_COUNT     2
#define D_NUM_OF_TASKS   2
#define D_STACK_SIZE     64
#define D_EVENT_BITS     0x51
#define D_AND            1
#define D_OR             2
#define D_CLEAR_BITS     4
#define D_WAIT_FOREVER   0
#define D_MAX_QUEUE_SIZE 5

/* task handler function definition */
typedef void (*task_handler)(void);
/* tasks stack */
unsigned int task0_stack[D_STACK_SIZE];
unsigned int task1_stack[D_STACK_SIZE];
unsigned int main_stack;

/* task list node */
typedef struct taskNode_t
{
  /* owner of this node */
  void *p_owner;
  /* next task node */
  struct taskNode_t *pNextTaskNode;
}taskNode_t;

/* task list */
typedef struct taskList
{
  /* link to the first task node */
  taskNode_t *pNextTaskNode;
  /* task pointed by this node */
  unsigned int node_count;
}taskList_t;

/* task control block */
typedef struct taskCB
{
  /* task stack address */
  void         *pStack;
  /* task handler function */
  task_handler  func;
  /* task node */
  taskNode_t node;
}taskCB_t;

/* semaphore control block */
typedef struct semaphoreCB
{
  /* semaphore counter */
  unsigned int  counter;
  /* semaphore max count */
  unsigned int  max_count;
  /* pending tasks */
  unsigned int  pending_tasks;
}semaphoreCB_t;

/* event control block */
typedef struct eventCB
{
  /* event bits */
  unsigned int  expected_bits;
  /* wait condition */
  unsigned int  expected_bits_state;
  /* pending tasks list */
  unsigned int  pending_tasks;
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
  unsigned int  pending_tasks;
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
static taskList_t    ready_tasks_list, pending_tasks_list;

taskCB_t g_tasks_list[D_NUM_OF_TASKS] = {
		{0, task0_func, { 0, 0 }},
		{0, task1_func, { 0, 0 }},
};

void add_task_to_list(taskList_t* pList, taskCB_t* p_task)
{
  taskNode_t *p_node;

  /* is the list empty */
  if (pList->node_count == 0)
  {
    /* set the list head */
    pList->pNextTaskNode = &p_task->node;
  }
  else
  {
    /* loop until we find the last item */
    p_node = pList->pNextTaskNode;
    while (p_node->pNextTaskNode)
    {
      p_node = p_node->pNextTaskNode;
    }
    /* last item should point to current task */
    p_node->pNextTaskNode = &p_task->node;
  }

  /* now this is the last item in the list */
  p_task->node.pNextTaskNode = 0;

  /* increment nodes count */
  pList->node_count++;
}

/*
 * remove the top node from a given list and update the ready task list
 * pList  - the list to remove from
 * return the address of the moved node
 */
taskNode_t* remove_head_from_list(taskList_t* pList)
{
  taskNode_t *p_node;

  /* is the list empty */
  if (pList->node_count == 0)
  {
    /* nothing to remove */
    return 0;
  }

  /* get the address of the node to be removed */
  p_node = pList->pNextTaskNode;
  /* update the new list head */
  pList->pNextTaskNode = p_node->pNextTaskNode;
  /* clear the 'next node' of the removed head */
  p_node->pNextTaskNode = 0;
  /* decrement nodes count */
  pList->node_count--;

  return p_node;
}

/*
 * Read event bits
 * p_event - event handle
 * get_bits - expected bits
 * bits_condition - can be D_AND/D_OR and D_CLEAR_BITS
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
    if ((bits_condition & D_AND && bits == get_bits) || ((bits_condition & D_OR && bits)))
    {
      /* do we need to clear the bits */
      if (bits_condition & D_CLEAR_BITS)
      {
         p_event->expected_bits &= ~bits;
      }
      /* we got what we wanted */
      break;
    }
    /* for a zero wait_time, we only switch context w/o
       any real timer */
    else if (wait_time == 0)
    {
      /* add current task to be pending */
      add_task_to_list(&pending_tasks_list, g_p_current_task);
      /* mark we have a waiting list */
      p_event->pending_tasks++;
      /* switch to other task */
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

  /* return the bits we got */
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
  taskNode_t* p_node;

  /* set the bits */
  p_event->expected_bits |= set_bits;
  /* check if there are pending tasks */
  if (p_event->pending_tasks != 0)
  {
    /* remove the pending task from the list */
    p_node = remove_head_from_list(&pending_tasks_list);
    /* add the removed node to the ready task list */
    add_task_to_list(&ready_tasks_list, p_node->p_owner);
    /* add g_p_current_task to the ready task list (needed for the simulation) */
    add_task_to_list(&ready_tasks_list, g_p_current_task);
    /* if no other pending tasks */
    p_event->pending_tasks--;
    /* switch to other task */
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
      /* decrement counter */
      p_sem->counter--;
      /* semaphore is taken */
      return 1;
    }
    /* for a zero wait_time, we only switch context w/o
       any real timer */
    else if (wait_time == D_WAIT_FOREVER)
    {
      /* add current task to be pending */
      add_task_to_list(&pending_tasks_list, g_p_current_task);
      /* mark we have a waiting list */
      p_sem->pending_tasks++;
      /* switch to other task */
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
  taskNode_t* p_node;

  /* verify semaphore counter */
  if (p_sem->counter < p_sem->max_count)
  {
    /* increment counter */
    p_sem->counter++;
    /* if tasks are pending this semaphore */
    if (p_sem->pending_tasks != 0)
    {
      /* remove the pending task from the list */
      p_node = remove_head_from_list(&pending_tasks_list);
      /* add the removed node to the ready task list */
      add_task_to_list(&ready_tasks_list, p_node->p_owner);
      /* add g_p_current_task to the ready task list (needed for the simulation) */
      add_task_to_list(&ready_tasks_list, g_p_current_task);
      /* decrement pending tasks */
      p_sem->pending_tasks--;
      /* switch to other task */
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
    /* check if the queue is none empty */
    if (p_queue->num_of_items != 0)
    {
      /* read queue item */
      *p_item = p_queue->queue[p_queue->pop_index];
      /* increment pop index */
      p_queue->pop_index = (p_queue->pop_index + 1) % D_MAX_QUEUE_SIZE;
      /* decrement number of items in the queue */
      p_queue->num_of_items--;
      return 1;
    }
    /* for a zero wait_time, we only switch context w/o
       any real timer */
    else if (wait_time == D_WAIT_FOREVER)
    {
      /* add current task to be pending */
      add_task_to_list(&pending_tasks_list, g_p_current_task);
      /* mark we have a waiting task */
      p_queue->pending_tasks++;
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
  taskNode_t* p_node;

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
  if (p_queue->pending_tasks != 0)
  {
    /* remove the pending task from the list */
    p_node = remove_head_from_list(&pending_tasks_list);
    /* add the removed node to the ready task list */
    add_task_to_list(&ready_tasks_list, p_node->p_owner);
    /* add g_p_current_task to the ready task list (needed for the simulation) */
    add_task_to_list(&ready_tasks_list, g_p_current_task);
    /* decrement pending tasks */
    p_queue->pending_tasks--;
    /* switch to other task */
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
  /* add g_p_current_task to the ready task list (needed for the simulation) */
  add_task_to_list(&ready_tasks_list, g_p_current_task);
  /* switch to other task */
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
  event_get(&g_event, D_EVENT_BITS, D_OR | D_CLEAR_BITS, D_WAIT_FOREVER);
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
  /* if a task is already running */
  if (g_p_current_task != 0)
  {
    /* save current task sp */
    g_p_current_task->pStack = p_task_sp;
  }

  /* get the next ready task */
  g_p_current_task = (taskCB_t*)remove_head_from_list(&ready_tasks_list)->p_owner;

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
   p_sem->pending_tasks = 0;
}

/*
 * initialize event
 */
void init_event(eventCB_t* p_event)
{
  p_event->expected_bits = 0;
  p_event->expected_bits_state = 0;
  p_event->pending_tasks = 0;
}

/*
 * initialize queue
 */
void init_queue(queueCB_t* p_queue)
{
  p_queue->push_index = 0;
  p_queue->pop_index = 0;
  p_queue->num_of_items = 0;
  p_queue->pending_tasks = 0;
}

static int __attribute__ ((noinline))
benchmark_body (int rpt)
{
  unsigned char i, j;
  unsigned int* stack_array[D_NUM_OF_TASKS] = { task0_stack, task1_stack };

  for (j = 0 ; j < rpt ; j++)
  {
    /* clear the ready list (from previous run) */
    while (ready_tasks_list.node_count)
    {
      remove_head_from_list(&ready_tasks_list);
    }
    /* initialize the task and stack of each task */
    for (i = 0 ; i < D_NUM_OF_TASKS ; i++)
    {
      g_tasks_list[i].pStack = initialize_task_stack(g_tasks_list[i].func, (unsigned char*)stack_array[i] + 4*(D_STACK_SIZE - 1));
      g_tasks_list[i].node.p_owner = &g_tasks_list[i];
      add_task_to_list(&ready_tasks_list, &g_tasks_list[i]);
    }

    /* task 0 is ready for execution */
    //add_task_to_list(&ready_tasks_list, &g_tasks_list[0]);
    /* task 1 is pending */
    //add_task_to_list(&pending_tasks_list, &g_tasks_list[1]);
    g_p_current_task = 0;

    init_semaphore(&g_sem);
    init_event(&g_event);
    init_queue(&g_queue);
    invoke_first_task();
  }

  return 0;
}

/*
   Local Variables:
   mode: C
   c-file-style: "gnu"
   End:
*/
