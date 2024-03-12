obj-m += memory.o
SRC := $(shell pwd)
KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build

ifeq ($(origin KERNEL_SRC), undefined)
KERNEL_SRC := /lib/modules/$(shell uname -r)/build
endif

all:
    $(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules
    depmode -a

clean:
    $(MAKE) -C $(KERNEL_SRC) M=$(SRC) clean
