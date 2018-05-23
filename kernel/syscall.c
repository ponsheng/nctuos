#include <kernel/task.h>
#include <kernel/timer.h>
#include <kernel/mem.h>
#include <kernel/cpu.h>
#include <kernel/syscall.h>
#include <kernel/trap.h>
#include <inc/stdio.h>

void do_puts(char *str, uint32_t len)
{
	uint32_t i;
	for (i = 0; i < len; i++)
	{
		k_putch(str[i]);
	}
}

int32_t do_getc()
{
	return k_getc();
}

int32_t do_syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	int32_t retVal = -1;
	//extern Task *cur_task;

	switch (syscallno)
	{
	case SYS_fork:
		/* Lab 5
     * You can reference kernel/task.c, kernel/task.h
     */
        retVal = sys_fork();
		break;

	case SYS_getc:
		retVal = do_getc();
		break;

	case SYS_puts:
		do_puts((char*)a1, a2);
		retVal = 0;
		break;

	case SYS_getpid:
		/*  Lab 5
     * Get current task's pid
     */
        retVal = cur_task->task_id;
		break;

	case SYS_getcid:
		/* Lab6: get current cpu's cid */
		retVal = thiscpu->cpu_id;
		break;

	case SYS_sleep:
		/* Lab 5
     * Yield this task
     * You can reference kernel/sched.c for yielding the task
     */
        cur_task->remind_ticks = a1;
        cur_task->state = TASK_SLEEP;
        sched_yield();
		break;

	case SYS_kill_self:
        sys_kill(cur_task->task_id);
        break;

	case SYS_kill:
		/* Lab 5
     * Kill specific task
     * You can reference kernel/task.c, kernel/task.h
     */
        sys_kill(a1);
		break;

  case SYS_get_num_free_page:
		/* Lab 5
     * You can reference kernel/mem.c
     */
    retVal = sys_get_num_free_page();
    break;

  case SYS_get_num_used_page:
		/* Lab 5
     * You can reference kernel/mem.c
     */
    retVal = sys_get_num_used_page();
    break;

  case SYS_get_ticks:
		/* Lab 5
     * You can reference kernel/timer.c
     */
    retVal = sys_get_ticks();
    break;

  case SYS_settextcolor:
		/* Lab 5
     * You can reference kernel/screen.c
     */
        sys_settextcolor(a1, a2);
    break;

  case SYS_cls:
		/* Lab 5
     * You can reference kernel/screen.c
     */
        sys_cls();
    break;

	}
	return retVal;
}

static void syscall_handler(struct Trapframe *tf)
{
	/* Lab5
   * call do_syscall
   * Please remember to fill in the return value
   * HINT: You have to know where to put the return value
   */
/*struct PushRegs {
	uint32_t reg_edi;
	uint32_t reg_esi;
	uint32_t reg_ebp;
	uint32_t reg_oesp;
	uint32_t reg_ebx;
	uint32_t reg_edx;
	uint32_t reg_ecx;
	uint32_t reg_eax;
}*/

    uint32_t num, a1, a2, a3, a4, a5, ret;
    struct PushRegs* regs = &(tf->tf_regs);
    num = regs->reg_eax;
    a1 = regs->reg_edx;
    a2 = regs->reg_ecx;
    a3 = regs->reg_ebx;
    a4 = regs->reg_edi;
    a5 = regs->reg_esi;
    ret = do_syscall(num, a1, a2, a3, a4, a5);

//    asm volatile(""
//            : "=a" (ret));

    regs->reg_eax = ret;
}

void syscall_init()
{
  /* Lab5
   * Please set gate of system call into IDT
   * You can leverage the API register_handler in kernel/trap.c
   * NOTE dpl 3
   */
    extern void isr_syscall();
    register_handler(T_SYSCALL, syscall_handler, isr_syscall, 1, 3);
}

