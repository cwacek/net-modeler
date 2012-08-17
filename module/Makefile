
obj-m :=  net-modeler.o
net-modeler-objs := nm_main.o nm_scheduler.o nm_structures.o nm_proc.o
KDIR := /lib/modules/$(shell uname -r)/build

all: module

module:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean
	
tags:
	ctags *.c *.h

install: all uninstall
	sudo insmod net-modeler.ko

uninstall:
	-sudo rmmod net-modeler.ko
