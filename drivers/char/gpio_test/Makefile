#
# Makefile for sun6i gpio test driver
#

PWD=$(shell pwd)

obj-$(CONFIG_GPIO_TEST_DRIVER)+=sun6i_gpio_test.o 		\

install: build
	cp sun6i_gpio_test.ko $(LICHEE_MOD_DIR)/

build:
	@echo $(LICHEE_KDIR)
	$(MAKE) -C $(LICHEE_KDIR) M=$(PWD)

clean:
	@rm -rf *.o *.ko .*.cmd *.mod.c *.order *.symvers .tmp_versions *~
