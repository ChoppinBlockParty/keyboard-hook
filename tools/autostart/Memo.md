To make Writer (linux kernel module) autoloadable

1. gzip -9 LinuxKeyboardHookWriter.ko
2. sudo cp LinuxKeyboardHookWriter.ko.gz /lib/modules/extramodules-3.14-lts/
3. depmod (default is -a)

For Reader

1. cp LinuxKeyboardHookReader /usr/bin
2. sudo nohup LinuxKeyboardHookReader
