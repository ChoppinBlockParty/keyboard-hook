#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/version.h>

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#include <asm/uaccess.h>

#include "DeviceInfoBuffer.h"
#include "OutputKeyboard.h"

MODULE_AUTHOR("Viacheslav Mikerov");
MODULE_LICENSE("GPL");

#define LINUX_KEYBOARD_HOOK_WRITER_MODULE_NAME "LinuxKeyboardHookWriter"

static int const _deviceCount = 2;

static unsigned int  _major = 0;
static struct class* _class = NULL;

static void
cfake_cleanup_module(void) {
  if (_class) {
    class_destroy(_class);
  }

  unregister_chrdev_region(MKDEV(_major, 0), _deviceCount);
  return;
}

static int __init
cfake_init_module(void) {
  int   err = 0;
  dev_t dev = 0;

  if (_deviceCount <= 0) {
    printk(KERN_WARNING "[target] Invalid value of _deviceCount: %d\n",
           _deviceCount);
    err = -EINVAL;
    return err;
  }

  /* Get a range of minor numbers (starting with 0) to work with */
  err = alloc_chrdev_region(&dev,
                            0,
                            _deviceCount,
                            LINUX_KEYBOARD_HOOK_WRITER_MODULE_NAME);

  if (err < 0) {
    printk(KERN_WARNING "[target] alloc_chrdev_region() failed\n");
    return err;
  }

  _major = MAJOR(dev);

  /* Create device class (before allocation of the array of devices) */
  _class = class_create(THIS_MODULE, LINUX_KEYBOARD_HOOK_WRITER_MODULE_NAME);

  if (IS_ERR(_class)) {
    err = PTR_ERR(_class);
    cfake_cleanup_module();
    return err;
  }

  err = createDeviceInfoBuffer(_major, 0, _class);

  if (err != 0) {
    cfake_cleanup_module();
    return err;
  }

  return 0;       /* success */
}

static void __exit
cfake_exit_module(void) {
  releaseDeviceInfoBuffer();
  cfake_cleanup_module();
  return;
}

module_init(cfake_init_module);
module_exit(cfake_exit_module);
/* ================================================================ */
