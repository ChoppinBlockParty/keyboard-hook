To make Writer (linux kernel module) autoloadable

1. gzip -9 keyboard_hook_writer.ko
2. sudo cp keyboard_hook_writer.ko.gz /lib/modules/extramodules-3.14-lts/
3. depmod (default is -a)
4. put keyboard_hook_writer.conf to /etc/modules-load.d

For Reader

1. cp KeyboardHookReader /usr/bin
2. sudo nohup KeyboardHookReader > null&
