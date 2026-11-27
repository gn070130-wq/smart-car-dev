KERNELDIR := /home/wuwu/loongxin/linux/linux-6.9


CURRENT_PATH := $(shell pwd)
obj-m := encoder.o

build: kernel_modules


kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean