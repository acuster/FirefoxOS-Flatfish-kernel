#
# Makefile for sun7i dma test driver
#

PWD=$(shell pwd)

obj-$(CONFIG_DMA_TEST_DRIVER) += sunxi_dma_test.o
sunxi_dma_test-objs :=  test_case_normal.o	\
			test_case_dedicate.o	\
			test_case_other.o	\
			test_case_two_thread.o	\
			sun7i_dma_test.o 	\

install: build
	cp sunxi_dma_test.ko $(LICHEE_MOD_DIR)/

build:
	@echo $(LICHEE_KDIR)
	$(MAKE) -C $(LICHEE_KDIR) M=$(PWD)

clean:
	@rm -rf *.o *.ko .*.cmd *.mod.c *.order *.symvers .tmp_versions *~
