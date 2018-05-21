#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/shell.h>
#include <inc/timer.h>

extern int __text_start;
extern int __text_end;
extern int __data_start;
extern int __data_end;

struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "chgcolor", "Change text to specifed color", mon_chgcolor },
	{ "print_tick", "Display system tick", print_tick }
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))


int mon_help(int argc, char **argv)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int mon_chgcolor(int argc, char **argv)
{
	unsigned char forecolor = 0xFF;
	if ( argc < 2) {
		settextcolor(forecolor, 0);
		cprintf("No color code provided, reset color\n");
	} else {
		if ( argv[1][0]  >= 'a' && argv[1][0]  <= 'f' ) {
			forecolor = argv[1][0]- 'a' + 10;
		} else if ( argv[1][0]  >= 'A' && argv[1][0]  <= 'F' ) {
			forecolor = argv[1][0]- 'A' + 10;
		} else if ( argv[1][0]  >= '0' && argv[1][0]  <= '9' ) {
			forecolor = argv[1][0]- '0';
		}
		settextcolor(forecolor, 0);
		cprintf("Change to %x color\n", forecolor);
	}
	return 0;
}

int mon_kerninfo(int argc, char **argv)
{
	/* Print the kernel code and data section size 
   * NOTE: You can count only linker script (kernel/kern.ld) to
   *       provide you with those information.
   *       Use PROVIDE inside linker script and calculate the
   *       offset.
   */
	cprintf("Kernel Info\n");
	cprintf("Code section start: %p, end: %p\n", &__text_start, &__text_end);
	cprintf("Code section size: %d\n", &__text_end - &__text_start);
	cprintf("Data section start: %p, end: %p\n", &__data_start, &__data_end);
	cprintf("Data section size: %d\n", &__data_end - &__data_start);
	return 0;
}
int print_tick(int argc, char **argv)
{
	cprintf("Now tick = %d\n", get_tick());
	return 0;
}

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int runcmd(char *buf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}
void shell()
{
	char *buf;
	cprintf("Welcome to the OSDI course!\n");
	cprintf("Type 'help' for a list of commands.\n");

	while(1)
	{
		buf = readline("OSDI> ");
		if (buf != NULL)
		{
			if (runcmd(buf) < 0)
				break;
		}
	}
}
