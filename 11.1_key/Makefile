KERNELDIR := /home/boddy/imx6ull/ubootkernel/1_video/linux-imx-rel_imx_4.1.15_2.1.0_ga_alientek
CURRENT_PATH := $(shell pwd)
obj-m := nchrdrivetemp.o

build: kernel_modules

kernel_modules:
	bear -l libear.so $(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
	cp *.ko ~/env/rootfs/home/root

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
