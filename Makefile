# The object file to build
obj-m += virt_tty.o

# Standard make rules for external modules
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
