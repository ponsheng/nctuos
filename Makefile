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

IMG=Kernel.img

include boot/Makefile
include kernel/Makefile

all: boot/boot kernel/system
	dd if=/dev/zero of=$(OBJDIR)/$(IMG) count=10000 2>/dev/null
	dd if=$(OBJDIR)/boot/boot of=$(OBJDIR)/$(IMG) conv=notrunc 2>/dev/null
	dd if=$(OBJDIR)/kernel/system of=$(OBJDIR)/$(IMG) seek=1 conv=notrunc 2>/dev/null

clean:
	rm -rf $(OBJDIR)/boot/*.o $(OBJDIR)/boot/boot.out $(OBJDIR)/boot/boot $(OBJDIR)/boot/boot.asm
	rm -rf $(OBJDIR)/kernel/*.o $(OBJDIR)/kernel/system* kernel.*
	rm -rf $(OBJDIR)/lib/*.o
	rm -rf $(OBJDIR)/user/*.o
	rm -rf $(OBJDIR)/user/*.asm

QEMU_ARG= --curses

qemu:
	qemu-system-i386 -hda $(IMG) $(QEMU_ARG) #-monitor stdio

debug:
	qemu-system-i386 -hda $(IMG) -s -S $(QEMU_ARG) #-monitor stdio -s -S

