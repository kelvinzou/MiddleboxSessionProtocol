obj-m    := translator.o 
#ccflags-y := -I$(src)
#translator-y := pre_routing.c pre_routing.h post_routing.h post_routing.c
CXX = gcc 
KDIR    := /lib/modules/$(shell uname -r)/build
PWD    := $(shell pwd)


default: netlink_user
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules  
netlink_kernel: 
	$(CXX) -o netlink_kernel netlink_kernel.c

clean: cleanuser
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean 
cleanuser:
	rm -rf netlink_user *.o
