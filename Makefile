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

OBJDIR = .

CPUS ?= 1
IMG=Kernel.img

CPUS ?= 1

all: clean boot/boot kernel/system
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
QEMU_ARG := $(CURSE) -smp $(CPUS)


HDB := true
ifdef HDB

HDB := lab7.img
# run this first
# qemu-img create -f raw lab7.img 32M
QEMU_ARG := $(QEMU_ARG) -hdb $(HDB)

endif

	#qemu-system-i386 -hda kernel.img -hdb lab7.img -monitor stdio -smp $(CPUS)

qemu:
	qemu-system-i386 -hda $(IMG) $(QEMU_ARG) #-monitor stdio

debug:
	qemu-system-i386 -hda $(IMG) -s -S $(QEMU_ARG) #-monitor stdio
