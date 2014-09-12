obj-m    := outputFilter.o inputFilter.o extraFilter.o check_forwarding.o LOCAL_OUT.o

KDIR    := /lib/modules/$(shell uname -r)/build
PWD    := $(shell pwd)


default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
