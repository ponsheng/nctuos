#include <kernel/task.h>
#include <inc/x86.h>

// do while(0): avoid "multi-statement in macro" error
#define ctx_switch(ts) \
  do { env_pop_tf(&((ts)->tf)); } while(0)

/*Lab5
* Implement a simple round-robin scheduler (Start with the next one)
*
* 1. You have to remember the task you picked last time.
*
* 2. If the next task is in TASK_RUNNABLE state, choose
*    it.
*
* 3. After your choice set cur_task to the picked task
*    and set its state, remind_ticks, and change page
*    directory to its pgdir.
*
* 4. CONTEXT SWITCH, leverage the macro ctx_switch(ts)
*    Please make sure you understand the mechanism.
*/
void sched_yield(void)
{
	extern Task tasks[];
	extern Task *cur_task;

    int pid;
    int i;
    Task* pretask = cur_task;

    for ( pid = pretask->task_id + 1, i = 0; i < NR_TASKS; i++, pid++) {
        
        if (pid >= NR_TASKS) {
            pid -= NR_TASKS;
        }
        if ( tasks[pid].state == TASK_RUNNABLE) {
            cur_task = &tasks[pid];
            break;
        }
    }
    // FIXME What if no task? idle task??
    cur_task->state = TASK_RUNNING;
    cur_task->remind_ticks = TIME_QUANT;

    // if same process -> no ctx switch
    if (pretask->task_id != cur_task->task_id ) {
//       printk("#%d yield to %d\n", pretask->task_id, cur_task->task_id);
        
       lcr3(PADDR(cur_task->pgdir));
       ctx_switch(cur_task);
    } else {
        return;
    }
}
