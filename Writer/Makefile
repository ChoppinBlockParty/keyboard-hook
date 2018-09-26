obj-m := keyboard_hook_writer.o
keyboard_hook_writer-objs := source/keyboard_hook_writer.o source/output_keyboard.o source/device_info_buffer.o source/input_keyboard.o

ccflags-y := -O2 -I$(src)/source

modules:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
