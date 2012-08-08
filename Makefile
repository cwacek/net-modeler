
obj-m :=  nm_injector.o nm_scheduler.o net-modeler.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
all: revision module

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
