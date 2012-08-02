
obj-m := net-modeler.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

revision:
	@echo "\"$(shell git describe)\"" > version.i 

all: revision
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	-@rm *.ko *o 2>/dev/null
