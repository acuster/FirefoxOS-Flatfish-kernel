#!/bin/bash

make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- -j8

arm-linux-gnueabi-objcopy -R .note.gnu.build-id -S -O binary vmlinux bImage
