obj-m := mymodule.o
mymodule-objs := source/mymodule.o source/OutputKeyboard.o source/DeviceInfoBuffer.o source/InputKeyboard.o source/handleEvents.o

ccflags-y := -O2 -I$(src)/source

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
