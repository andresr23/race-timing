#---- Module
obj-m += pmc-module.o

#-------------------------------------------------------------------------------

.PHONY: intel amd clean

intel:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	echo "pmc-module.ko built for Intel."

amd:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) CFLAGS_MODULE=-DAMD modules
	echo "pmc-module.ko built for AMD."

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
