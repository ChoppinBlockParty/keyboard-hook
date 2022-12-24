Keyboard hook for Linux
=================================

- Replaces `;` with `<backspace>`
- Replaces `Alt-;` with `;`

and a few other replacements.

To make Writer (linux kernel module) autoloadable

``` shell
    gzip -9 keyboard_hook_writer.ko
    sudo cp keyboard_hook_writer.ko.gz /lib/modules/extramodules-3.14-lts/
    # Generate modules.dep and map files
    # default is -a (probe all modules)
    depmod
    sudo mv tools/autostart/keyboard_hook_writer.conf /etc/modules-load.d
```

For Reader

Do not forget `sudo`

``` shell
    sudo KeyboardHookReader -h
    sudo cp KeyboardHookReader /usr/bin
```

To restart in runtime

```bash
sudo pkill KeyboardHook; sudo rmmod keyboard_hook_writer; sudo modprobe keyboard_hook_writer; sudo /etc/keyboard-hook-service.sh
```

