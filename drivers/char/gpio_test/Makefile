#
# Makefile for sun7i gpio test driver
#

PWD=$(shell pwd)

obj-$(CONFIG_SUN7I_GPIO_TEST) += sun7i_gpio_test.o 		\

install: build
	cp sun7i_gpio_test.ko $(LICHEE_MOD_DIR)/

build:
	@echo $(LICHEE_KDIR)
	$(MAKE) -C $(LICHEE_KDIR) M=$(PWD)

clean:
	@rm -rf *.o *.ko .*.cmd *.mod.c *.order *.symvers .tmp_versions *~
