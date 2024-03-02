obj-m += memory.o

all:
        mkdir -p /lib/modules/$(shell uname -r)/build
        make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
        make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
