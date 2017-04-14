KDIR ?= /lib/modules/$(shell uname -r)/build

default: modules cdmctl

modules:
	$(MAKE) -C $(KDIR) M=$(PWD)/driver modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD)/driver clean
	$(RM) *.o cdmctl
