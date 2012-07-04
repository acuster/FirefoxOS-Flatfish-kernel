#
# Makefile for the linux kernel.
#

obj-y=core.o printk.o devices.o
obj-y += clock/
obj-y += dma/
obj-y += gpio/
obj-y += sys_config.o
obj-$(CONFIG_CPU_FREQ)			+= cpu-freq/

ifeq ($(CONFIG_SMP),y)
obj-y += platsmp.o headsmp.o
obj-$(CONFIG_HOTPLUG_CPU)		+= hotplug.o


ifeq ($(CONFIG_LOCAL_TIMERS),y)
obj-y += localtimer.o
endif


endif
