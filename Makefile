SUBDIRS = module

.PHONY: all

all:
	cd module; $(MAKE) 

module-install: 
	cd module ;$(MAKE) install

module-remove:
	cd module ; $(MAKE) uninstall

clean:
	-for d in $(SUBDIRS); do (cd $$d; $(MAKE) clean); done

	
