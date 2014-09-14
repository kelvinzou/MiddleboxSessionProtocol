obj-m    := translator.o 
#ccflags-y := -I$(src)
#translator-y := pre_routing.c pre_routing.h post_routing.h post_routing.c

KDIR    := /lib/modules/$(shell uname -r)/build
PWD    := $(shell pwd)


default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules 


clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean 