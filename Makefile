# Makefile for the simple kernel.
CC	=gcc
AS	=as
LD	=ld
OBJCOPY = objcopy
OBJDUMP = objdump
NM = nm

CFLAGS = -m32 -Wall -O0 -fstrength-reduce -fomit-frame-pointer -finline-functions -nostdinc -fno-builtin 
# Add debug symbol
CFLAGS += -g

CFLAGS += -I.

# Ignore warning
CFLAGS += -Wno-implicit-function-declaration

OBJDIR = .

CPUS ?= 1
IMG=Kernel.img

CPUS ?= 1

all: boot/boot kernel/system
	dd if=/dev/zero of=$(OBJDIR)/$(IMG) count=10000 2>/dev/null
	dd if=$(OBJDIR)/boot/boot of=$(OBJDIR)/$(IMG) conv=notrunc 2>/dev/null
	dd if=$(OBJDIR)/kernel/system of=$(OBJDIR)/$(IMG) seek=1 conv=notrunc 2>/dev/null

include boot/Makefile
include kernel/Makefile

clean:
	rm -rf $(OBJDIR)/boot/*.o $(OBJDIR)/boot/boot.out $(OBJDIR)/boot/boot $(OBJDIR)/boot/boot.asm
	rm -rf $(OBJDIR)/kernel/*.o $(OBJDIR)/kernel/system* kernel.*
	rm -rf $(OBJDIR)/lib/*.o
	rm -rf $(OBJDIR)/user/*.o
	rm -rf $(OBJDIR)/user/*.asm
	rm -rf $(OBJDIR)/kernel/fs/*.o $(OBJDIR)/kernel/fs/fat/*.o
	rm -rf $(OBJDIR)/kernel/drv/*.o

CURSE ?= --curses

# -monitor stdio

QEMU_ARG := $(CURSE) -smp $(CPUS)


HDB ?= true
ifdef HDB

HDB := lab7.img
QEMU_ARG := -hdb $(HDB) $(QEMU_ARG) 

# run this first
# qemu-img create -f raw lab7.img 32M
$(HDB):
	qemu-img create -f raw lab7.img 32M

hddump: $(HDB)
	hexdump -C -v $^

endif


qemu:
	qemu-system-i386 -hda $(IMG) $(QEMU_ARG)

debug:
	qemu-system-i386 -hda $(IMG) $(QEMU_ARG) -s -S
