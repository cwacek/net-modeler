
obj-m :=  net-modeler.o
net-modeler-objs := nm_main.o nm_scheduler.o nm_structures.o
KDIR := /lib/modules/$(shell uname -r)/build
#KDIR := /home/cwacek/scratch/kernel/linux-3.0.0
PWD := $(shell pwd)
U_SRCS := uspace_queue.c


all: revision module

#USERSPACE
U_CC := gcc
U_CFLAGS := -g 
U_LFLAGS := -L/usr/lib -lnetfilter_queue -lnfnetlink -lrt
LD_LIBRARY_PATH=/usr/lib

nm-userspace: userspace-obj 
	$(U_CC) $(U_CFLAGS)  -o $@ $(U_SRCS:.c=.o) $(U_LFLAGS)

userspace-obj: $(U_SRCS)
	$(U_CC) $(U_LFLAGS) $(U_CFLAGS) -c $(U_SRCS)

hpet_test: hpet_control.c
	$(U_CC) -DTEST=1 $(U_CFLAGS) $? -o $@ $(U_LFLAGS) 

hrtimer_test: hrtimer_test.c
	$(U_CC) $(U_CFLAGS) $? -o $@ $(U_LFLAGS) 

#KERNEL

module:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

revision:
	@echo "#define VERSION \"$(shell git describe --tags)\"" > version.i 

clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean
	
tags:
	ctags *.c *.h

install: all uninstall
	sudo insmod net-modeler.ko

uninstall:
	-sudo rmmod net-modeler.ko
