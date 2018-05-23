#include <inc/mmu.h>
#include <inc/types.h>
#include <inc/string.h>
#include <inc/x86.h>
#include <inc/memlayout.h>
#include <kernel/task.h>
#include <kernel/mem.h>
#include <kernel/cpu.h>

// Global descriptor table.
//
// Set up global descriptor table (GDT) with separate segments for
// kernel mode and user mode.  Segments serve many purposes on the x86.
// We don't use any of their memory-mapping capabilities, but we need
// them to switch privilege levels. 
//
// The kernel and user segments are identical except for the DPL.
// To load the SS register, the CPL must equal the DPL.  Thus,
// we must duplicate the segments for the user and the kernel.
//
// In particular, the last argument to the SEG macro used in the
// definition of gdt specifies the Descriptor Privilege Level (DPL)
// of that descriptor: 0 for kernel and 3 for user.
//
struct Segdesc gdt[NCPU + 5] =
{
	// 0x0 - unused (always faults -- for trapping NULL far pointers)
	SEG_NULL,

	// 0x8 - kernel code segment
	[GD_KT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 0),

	// 0x10 - kernel data segment
	[GD_KD >> 3] = SEG(STA_W, 0x0, 0xffffffff, 0),

	// 0x18 - user code segment
	[GD_UT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 3),

	// 0x20 - user data segment
	[GD_UD >> 3] = SEG(STA_W , 0x0, 0xffffffff, 3),

	// First TSS descriptors (starting from GD_TSS0) are initialized
	// in task_init()
	[GD_TSS0 >> 3] = SEG_NULL
	
};

struct Pseudodesc gdt_pd = {
	sizeof(gdt) - 1, (unsigned long) gdt
};



//static struct tss_struct tss;
Task tasks[NR_TASKS];

extern char bootstack[];

extern char UTEXT_start[], UTEXT_end[];
extern char UDATA_start[], UDATA_end[];
extern char UBSS_start[], UBSS_end[];
extern char URODATA_start[], URODATA_end[];
/* Initialized by task_init */
uint32_t UTEXT_SZ;
uint32_t UDATA_SZ;
uint32_t UBSS_SZ;
uint32_t URODATA_SZ;

//Task *cur_task = NULL; //Current running task


extern void sched_yield(void);

static void task_free(int pid);

/* Lab5
 * 1. Find a free task structure for the new task,
 *    the global task list is in the array "tasks".
 *    You should find task that is in the state "TASK_FREE"
 *    If cannot find one, return -1.
 *
 * 2. Setup the page directory for the new task
 *
 * 3. Setup the user stack for the new task, you can use
 *    page_alloc() and page_insert(), noted that the va
 *    of user stack is started at USTACKTOP and grows down
 *    to USR_STACK_SIZE, remember that the permission of
 *    those pages should include PTE_U
 *
 * 4. Setup the Trapframe for the new task
 *    We've done this for you, please make sure you
 *    understand the code.
 *
 * 5. Setup the task related data structure
 *    You should fill in task_id, state, parent_id,
 *    and its schedule time quantum (remind_ticks).
 *
 * 6. Return the pid of the newly created task.
 
 */
int task_create()
{
    int i;
	Task *ts = NULL;

	/* Find a free task structure */
    for ( i = 0; i < NR_TASKS; i++ ) {
        if ( tasks[i].state == TASK_FREE || tasks[i].state == TASK_STOP ) {
            ts = &(tasks[i]);
            break;
        }
    }
    if (!ts) return -1;

  /* Setup Page Directory and pages for kernel*/
  if (!(ts->pgdir = setupkvm()))
    panic("Not enough memory for per process page directory!\n");

  /* Setup User Stack */
    int ret;
    int size;
    struct PageInfo *page;
    for ( size = PGSIZE; size <= USR_STACK_SIZE; size += PGSIZE) {
        //printk("VM: %x\n", USTACKTOP);
        page = page_alloc(0);
        if ( ! page ) {
            goto create_error;
        }
        ret = page_insert(ts->pgdir, page,(void*) USTACKTOP - size, PTE_U | PTE_W );
        if ( ret ) goto create_error;
    }

	/* Setup Trapframe */
	memset( &(ts->tf), 0, sizeof(ts->tf));

	ts->tf.tf_cs = GD_UT | 0x03;
	ts->tf.tf_ds = GD_UD | 0x03;
	ts->tf.tf_es = GD_UD | 0x03;
	ts->tf.tf_ss = GD_UD | 0x03;
	ts->tf.tf_esp = USTACKTOP-PGSIZE;

	/* Setup task structure (task_id and parent_id) */
    ts->task_id = i;
    ts->parent_id = (cur_task) ? cur_task->task_id : 0 ;
    ts->state = TASK_RUNNABLE;
    ts->remind_ticks = TIME_QUANT;

    printk("task #%d created\n", i);
    return ts->task_id;

create_error:
    printk("task create failed\n");

    task_free(i);
    
    return -1;
}


/* Lab5
 * This function free the memory allocated by kernel.
 *
 * 1. Be sure to change the page directory to kernel's page
 *    directory to avoid page fault when removing the page
 *    table entry.
 *    You can change the current directory with lcr3 provided
 *    in inc/x86.h
 *
 * 2. You have to remove pages of USER STACK
 *
 * 3. You have to remove pages of page table
 *
 * 4. You have to remove pages of page directory
 *
 * HINT: You can refer to page_remove, ptable_remove, and pgdir_remove
 */


static void task_free(int pid)
{
    // Find task
    Task* ts = &(tasks[pid]);


    // Change to kernel pgdir
    lcr3(PADDR(kern_pgdir));

    int size;
    for ( size = PGSIZE; size <= USR_STACK_SIZE; size += PGSIZE) {
        page_remove(ts->pgdir, (void*) USTACKTOP - size);
    }
    // FIXME why not kernel pages

    // change to kernel page_dir
    ptable_remove(ts->pgdir);
    pgdir_remove(ts->pgdir);
}

// Lab6 TODO
//
// Modify it so that the task will be removed form cpu runqueue
// ( we not implement signal yet so do not try to kill process
// running on other cpu )
//
// this does not kill process #0
void sys_kill(int pid)
{
	if (pid > 0 && pid < NR_TASKS)
	//if (pid > 0 && pid < NR_TASKS)
	{
        //printk("Killed #%d\n", pid);
	    // Lab 5
        // Remember to change the state of tasks
        tasks[pid].state = TASK_STOP;
        //Free the memory
        task_free(pid);
        //and invoke the scheduler for yield
        if ( cur_task->task_id == pid ) {
           sched_yield(); 
        }
	}
}


/* Lab 5
 * In this function, you have several things todo
 *
 * 1. Use task_create() to create an empty task, return -1
 *    if cannot create a new one.
 *
 * 2. Copy the trap frame of the parent to the child
 *
 * 3. Copy the content of the old stack to the new one,
 *    you can use memcpy to do the job. Remember all the
 *    address you use should be virtual address.
 *
 * 4. Setup virtual memory mapping of the user prgram 
 *    in the new task's page table.
 *    According to linker script, you can determine where
 *    is the user program. We've done this part for you,
 *    but you should understand how it works.
 *
 * 5. The very important step is to let child and 
 *    parent be distinguishable!
 *
 * HINT: You should understand how system call return
 * it's return value.
 */

//
// Lab6 TODO:
//
// Modify it so that the task will disptach to different cpu runqueue
// (please try to load balance, don't put all task into one cpu)
//
int sys_fork()
{
    int pid;
    int size;
    struct PageInfo *page;
    void* ptr;
    Task* ts;
  /* pid for newly created process */
   if ( ! ( pid = task_create() )) {
        return -1;
   }
    ts = &tasks[pid];

    // copy tf
    ts->tf = cur_task->tf;

   // TODO setup stack
   // My strategy
   // 1. map new process's stack (pa) to somewhere in va (I use UTEMP)
   // 2. memcopy
   // 3. unmap
   // 4. finish
    for ( size = PGSIZE; size <= USR_STACK_SIZE; size += PGSIZE) {
        // Target va
        ptr = (void*)(USTACKTOP - size);
        //printk("Copying va: %p\n", ptr);

        // Get pa of forked task stack
        page = page_lookup(ts->pgdir, ptr, NULL);
        if (!page) {
            printk("fork failed ummap va: %p\n", ptr);
            return -1;
        }
        // map to UTEMP
        page_insert(cur_task->pgdir, page, UTEMP - PGSIZE, PTE_W);

        // Copy
        //printk("%p to %p start \n", ptr, UTEMP - PGSIZE);
        memcpy( UTEMP - PGSIZE, ptr, PGSIZE);

        //printk("%p to %p  end \n", ptr, UTEMP - PGSIZE);
        // Unmap
        page_remove(cur_task->pgdir, UTEMP - PGSIZE); 
    }
  
	if ((uint32_t)cur_task)
	{
    // Step 4: All user program use the same code for now 
    setupvm(tasks[pid].pgdir, (uint32_t)UTEXT_start, UTEXT_SZ);
    setupvm(tasks[pid].pgdir, (uint32_t)UDATA_start, UDATA_SZ);
    setupvm(tasks[pid].pgdir, (uint32_t)UBSS_start, UBSS_SZ);
    setupvm(tasks[pid].pgdir, (uint32_t)URODATA_start, URODATA_SZ);

	}

    // set return value
    tasks[pid].tf.tf_regs.reg_eax = 0;
    //ts->state = TASK_STOP;
    return pid;
}

/* Lab5
 * We've done the initialization for you,
 * please make sure you understand the code.
 */
void task_init()
{
  extern int user_entry();
	int i;
  UTEXT_SZ = (uint32_t)(UTEXT_end - UTEXT_start);
  UDATA_SZ = (uint32_t)(UDATA_end - UDATA_start);
  UBSS_SZ = (uint32_t)(UBSS_end - UBSS_start);
  URODATA_SZ = (uint32_t)(URODATA_end - URODATA_start);

	/* Initial task sturcture */
	for (i = 0; i < NR_TASKS; i++)
	{
		memset(&(tasks[i]), 0, sizeof(Task));
		tasks[i].state = TASK_FREE;

	}
	task_init_percpu();
}

// Lab6 TODO
//
// Please modify this function to:
//
// 1. init idle task for non-booting AP 
//    (remember to put the task in cpu runqueue) 
//
// 2. init per-CPU Runqueue
//
// 3. init per-CPU system registers
//
// 4. init per-CPU TSS
//
void task_init_percpu()
{
    	
   // Task *cur_task = NULL; //Current running task
    //#define cur_task  (cpus[cpunum()].cpu_task)

	int i;
	extern int user_entry();
	extern int idle_entry();
    int cpu = cpunum();
    struct tss_struct *tss = &(cpus[cpu].cpu_tss);
	// Setup a TSS so that we get the right stack
	// when we trap to the kernel.
	memset(tss, 0, sizeof(struct tss_struct));
	//tss->ts_esp0 = (uint32_t)bootstack + KSTKSIZE;
	tss->ts_esp0 = (uint32_t)(percpu_kstacks[cpu] + KSTKSIZE);
	tss->ts_ss0 = GD_KD;

	// fs and gs stay in user data segment
	tss->ts_fs = GD_UD | 0x03;
	tss->ts_gs = GD_UD | 0x03;

	/* Setup TSS in GDT */
	gdt[(GD_TSS0 >> 3) + cpu] = SEG16(STS_T32A, (uint32_t)(tss), sizeof(struct tss_struct), 0);
	gdt[(GD_TSS0 >> 3) + cpu].sd_s = 0;

	/* Setup first task */
	i = task_create();
	cur_task = &(tasks[i]);
//#define cur_task   ((Task*) &(cpus[cpunum()]->cpu_task))

	/* For user program */
	setupvm(cur_task->pgdir, (uint32_t)UTEXT_start, UTEXT_SZ);
	setupvm(cur_task->pgdir, (uint32_t)UDATA_start, UDATA_SZ);
	setupvm(cur_task->pgdir, (uint32_t)UBSS_start, UBSS_SZ);
	setupvm(cur_task->pgdir, (uint32_t)URODATA_start, URODATA_SZ);
    if ( cpu ==  bootcpu->cpu_id ) {
	    cur_task->tf.tf_eip = (uint32_t)user_entry;
    } else {
	    cur_task->tf.tf_eip = (uint32_t)idle_entry;
    }

	/* Load GDT&LDT */
	lgdt(&gdt_pd);


	lldt(0);

	// Load the TSS selector 
	ltr(GD_TSS0 + (cpu << 3));

	cur_task->state = TASK_RUNNING;
    printk("task_init suceed\n");
}
