To make Writer (linux kernel module) autoloadable

1. gzip -9 LinuxKeyboardHookWriter.ko
2. sudo cp LinuxKeyboardHookWriter.ko.gz /lib/modules/extramodules-3.14-lts/
3. depmod (default is -a)
4. put LinuxKeyboardHookWriter.conf to /etc/modules-load.d

For Reader

1. cp LinuxKeyboardHookReader /usr/bin
2. sudo nohup LinuxKeyboardHookReader > null&
