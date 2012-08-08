
obj-m :=  nm_injector.o nm_scheduler.o net-modeler.o
KDIR := /lib/modules/$(shell uname -r)/build
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
	@echo "\"$(shell git describe)\"" > version.i 

clean:
	-@rm *.ko *o 2>/dev/null

install: all
	sudo insmod net-modeler.ko

uninstall:
	-sudo rmmod net-modeler.ko
