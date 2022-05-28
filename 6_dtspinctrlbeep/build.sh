#!/bin/bash
bear -l libear.so make
rm -vf ~/env/rootfs/home/root/*.ko
rm -vf ~/env/rootfs/home/root/testapp.bin
cp -v ./*.ko ~/env/rootfs/home/root/
cp -v ./testapp/testapp.bin ~/env/rootfs/home/root
