KDIR ?= /lib/modules/$(shell uname -r)/build

default: modules cdmctl migrate_pages

migrate_pages: LDFLAGS = -lnuma

modules:
	$(MAKE) -C $(KDIR) M=$(PWD)/driver modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD)/driver clean
	$(RM) *.o cdmctl migrate_pages
