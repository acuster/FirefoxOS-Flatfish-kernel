#
# Makefile for the linux kernel.
#

obj-y=core.o clock.o printk.o devices.o

ifeq ($(CONFIG_SMP),y)
obj-y += platsmp.o headsmp.o
obj-$(CONFIG_HOTPLUG_CPU)		+= hotplug.o


ifeq ($(CONFIG_LOCAL_TIMERS),y)
obj-y += localtimer.o
endif

endif
