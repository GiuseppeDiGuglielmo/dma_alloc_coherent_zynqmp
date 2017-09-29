# dma_alloc_coherent_zynqmp

Device drivers that use dma_alloc_coherent (contiguous memory allocation) are relatively easy to be implemented on Xilinx Zynq-7000 boards (ARM Cortex-A9, 32bit), but they are not on Xinlinx ZynqMP boards (ARM Cortex-A53, 64bit). This example will showcase how to write a device driver for both the architectures. The work is on-going. Currently, `dma_alloc_coherent` returns `NULL` on the ARM 64bit CPU.

Thanks to Massimiliano Giacometti (max.giacometti@gmail.com) for an initial version of this code.

## How To

These are the steps to test the device driver on Xilinx Zynq-7000 boards, i.e. ARM Cortex-A9, 32bit architecture. You need the `arm-linux-gnueabihf-` cross-compilers in your environment. You can either use Vivado SDK or your own. You need the target kernel as well. I use Vivado Petalinux to cross-compiled Linux for my Zynq ZC702. 

```
$ SYSROOT= <...> /petalinux/xilinx-zc702-2017.2/build/tmp/sysroots/plnx_arm/
$ KERNEL_SRC_DIR= <...> /petalinux/xilinx-zc702-2017.2/build/tmp/work/plnx_arm-xilinx-linux-gnueabi/linux-xlnx/4.9-xilinx-v2017.2+gitAUTOINC+5d029fdc25-r0/linux-plnx_arm-standard-build/ 
$ cd dev
$ make clean
$ make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- KERNEL_SRC_DIR=$KERNEL_SRC_DIR MODULE=memalloc
$ cd ../test
$ make clean
$ make CROSS_COMPILE=arm-linux-gnueabihf- EXEC=memalloc_test SYSROOT=$SYSROOT
```

Now you can copy (`scp`) both the device driver (`*.ko`) and the test program on Linux that runs on the Xilinx Zynq-7000 board.

```
# insmod memalloc.ko
# ./memalloc_test
```

The same flow can be repeated for a Xinlinx ZynqMP board, ZynqMP ZCU102 in my case. Please, notice that you have to use the aarch64-linux-gnu- cross-compiler and a different target kernel.
```
$ SYSROOT= <...> /petalinux/xilinx-zcu102-2017.2/build/tmp/sysroots/plnx_aarch64/
$ KERNEL_SRC_DIR= <...> /petalinux/xilinx-zcu102-2017.2/build/tmp/work/plnx_aarch64-xilinx-linux/linux-xlnx/4.9-xilinx-v2017.2+gitAUTOINC+5d029fdc25-r0/linux-plnx_aarch64-standard-build/ 
$ cd dev
$ make clean
$ make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- KERNEL_SRC_DIR=$KERNEL_SRC_DIR MODULE=memalloc
$ cd ../test
$ make clean
$ make CROSS_COMPILE=aarch64-linux-gnu- EXEC=memalloc_test SYSROOT=$SYSROOT
```

In this case the test program will fail on the ARM 64bit CPU. The issue seems to be on the `dma_alloc_coherent` call that returns `NULL`.
