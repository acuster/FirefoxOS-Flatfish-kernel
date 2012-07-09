#
# Makefile for sun6i dma test driver
#

PWD=$(shell pwd)

obj-$(CONFIG_DMA_TEST_DRIVER)+=test_case_1t_mem_2_mem.o	\
			test_case_2t_mem_2_mem.o	\
			test_case_other.o		\
			test_case_single_md.o	\
			test_case_single_conti_md.o	\
			sun6i_dma_test.o 		\

install: build
	cp sun6i_dma_test.ko $(LICHEE_MOD_DIR)/

build:
	@echo $(LICHEE_KDIR)
	$(MAKE) -C $(LICHEE_KDIR) M=$(PWD)

clean:
	@rm -rf *.o *.ko .*.cmd *.mod.c *.order *.symvers .tmp_versions *~
