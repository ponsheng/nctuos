#ifndef TASK_H
#define TASK_H

#include <inc/trap.h>
#include <kernel/mem.h>
#define NR_TASKS	30
#define TIME_QUANT	100


typedef enum
{
	TASK_FREE = 0,
	TASK_RUNNABLE,
	TASK_RUNNING,
	TASK_SLEEP,
	TASK_STOP,
} TaskState;

// Each task's user space
#define USR_STACK_SIZE	(40960)

typedef struct Task
{
	int task_id;
	int parent_id;
	struct Trapframe tf; //Saved registers
	int32_t remind_ticks;
	TaskState state;	//Task state
  pde_t *pgdir;  //Per process Page Directory
  // Lab6
  struct Task  *next_task;	
} Task;

// Lab6 replace global symbol
#define cur_task  (cpus[cpunum()].cpu_task)

// Lab6
// 
// Design your Runqueue structure for cpu
// your runqueue sould have at least two
// variables:
//
// 1. an index for Round-robin scheduling
//
// 2. a list indicate the tasks in the runqueue
//
typedef struct
{
  int count;
  //circular  list->[A]->[B]->[C]->[A]
  Task *task_list;
  Task *task_list_tail;  // Use this to record idle
} Runqueue;


void task_init();
void task_init_percpu();
void env_pop_tf(struct Trapframe *tf);

/* Lab 5
 * Interface for real implementation of kill and fork
 * Since their real implementation should be in kernel/task.c
 */

int32_t sys_fork();
void sys_kill(int pid);
#endif
