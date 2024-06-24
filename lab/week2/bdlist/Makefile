obj-m+=bdlist.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ KBUILD_EXTMOD=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build/ KBUILD_EXTMOD=$(PWD) clean
