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

#include "device_info_buffer.h"
#include "output_keyboard.h"

MODULE_AUTHOR("Yuki");
MODULE_LICENSE("Dual BSD/GPL");

#define KEYBOARD_HOOK_WRITER_MODULE_NAME "keyboard_hook_writer"

static int const _deviceCount = MAX_NUMBER_OF_DEVICES + 1;

static unsigned int _major = 0;
static struct class* _class = NULL;

static void cfake_cleanup_module(void) {
  if (_class) {
    class_destroy(_class);
  }

  unregister_chrdev_region(MKDEV(_major, 0), _deviceCount);
  return;
}

static int __init cfake_init_module(void) {
  int err = 0;
  dev_t dev = 0;

  if (_deviceCount <= 0) {
    printk(KERN_WARNING "[target] Invalid value of _deviceCount: %d\n", _deviceCount);
    err = -EINVAL;
    return err;
  }

  /* Get a range of minor numbers (starting with 0) to work with */
  err
    = alloc_chrdev_region(&dev, 0, _deviceCount, KEYBOARD_HOOK_WRITER_MODULE_NAME);

  if (err < 0) {
    printk(KERN_WARNING "[target] alloc_chrdev_region() failed\n");
    return err;
  }

  _major = MAJOR(dev);

  /* Create device class (before allocation of the array of devices) */
  _class = class_create(THIS_MODULE, KEYBOARD_HOOK_WRITER_MODULE_NAME);

  if (IS_ERR(_class)) {
    err = PTR_ERR(_class);
    cfake_cleanup_module();
    return err;
  }

  err = create_device_info_buffer(_major, 0, _class);

  if (err != 0) {
    cfake_cleanup_module();
    return err;
  }

  return 0; /* success */
}

static void __exit cfake_exit_module(void) {
  destroy_device_info_buffer();
  cfake_cleanup_module();
  return;
}

module_init(cfake_init_module);
module_exit(cfake_exit_module);
