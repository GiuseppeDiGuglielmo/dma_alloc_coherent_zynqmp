ARCH            ?= arm64
KERNEL_SRC_DIR  ?= /lib/modules/$(shell uname -r)/build

ifeq ($(shell uname -m | sed -e s/arm64.*/arm64/),arm64)
else
	CROSS_COMPILE  ?= aarch64-linux-gnu-
endif

obj-m := $(MODULE).o

all:
	make -C $(KERNEL_SRC_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) modules

clean:
	make -C $(KERNEL_SRC_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) clean

