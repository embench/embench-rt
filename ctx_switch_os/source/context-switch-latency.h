#ifndef __CONTEXT_SWITCH_LATENCY_H__
#define __CONTEXT_SWITCH_LATENCY_H__

typedef unsigned int cycles_t;

/* functions implemented int context-switch-latency-rv.S */
void return_to_main(void);
void context_switch(void);
void invoke_first_task(void);
void* initialize_task_stack(void* p_func_handler, void* p_stack_address);

#endif /* __CONTEXT_SWITCH_LATENCY_H__ */
