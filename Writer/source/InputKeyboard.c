#include "InputKeyboard.h"

#include <linux/input.h>

#include "OutputKeyboard.h"

#define LINUX_KEYBOARD_HOOK_WRITER_INPUT_KEYBOARD_DEVICE_NAME \
  "LinuxKeyboardHookWriterInputKeyboard"

struct InputKeyboard* _keyboard;

struct InputKeyboard*
getInputKeyboard(void) {
  return _keyboard;
}

int
_openInputKeyboard(struct inode* inode, struct file* filp) {
  unsigned int mj = imajor(inode);
  unsigned int mn = iminor(inode);

  if (mj != _keyboard->major ||
      mn != _keyboard->minor) {
    printk(KERN_ERR "[target] "
                    "No device found with minor=%d and major=%d\n",
           mj, mn);
    return -ENODEV;             /* No such device */
  }

  if (inode->i_cdev != &_keyboard->cdev) {
    printk(KERN_ERR "[target] open: internal error\n");
    return -ENODEV;             /* No such device */
  }

  return 0;
}

ssize_t
_writeToInputKeyboard(struct file*       filp,
                      const char __user* buf,
                      size_t             count,
                      loff_t*            f_pos) {
  ssize_t retval = 0;

  if (count != _keyboard->blockSize) {
    printk(KERN_ERR "Value size is not equal to buffer block size\n");
    return 111;
  }

  mutex_lock(&_keyboard->mutex);
  // if (mutex_lock_killable(&_keyboard->mutex)) {
  //  printk(KERN_ERR "_writeToInputKeyboard: Failed to get lock\n");
  //  return -EINTR;
  // }

  if ((_keyboard->bufferPosition + count) >= _keyboard->buffer_size) {
    _keyboard->bufferPosition = 0;

    if ((_keyboard->bufferPosition + count) >= _keyboard->buffer_size) {
      printk(KERN_ERR "Out of memory request\n");
      retval = 1100;
      goto out;
    }
  }

  if (copy_from_user(&(_keyboard->data[_keyboard->bufferPosition]),
                     buf,
                     count) != 0) {
    printk(KERN_ERR "Failed to get data from user\n");
    retval = -EFAULT;
    goto out;
  }

  _keyboard->bufferPosition += count;
  *f_pos                     = _keyboard->bufferPosition;
  retval                     = count;

out:
  mutex_unlock(&_keyboard->mutex);
  return retval;
}

int
_releaseInputKeyboard(struct inode* inode, struct file* filp) {
  mutex_lock(&_keyboard->mutex);
  _keyboard->bufferPosition     = 0;
  _keyboard->bufferReadPosition = 0;
  mutex_unlock(&_keyboard->mutex);
  releaseOutputKeyboard();
  return 0;
}

struct file_operations InputKeyboardFops = {
  .owner   = THIS_MODULE,
  .open    = _openInputKeyboard,
  .write   = _writeToInputKeyboard,
  .release = _releaseInputKeyboard,
};

int
createInputKeyboard(unsigned int  major,
                    unsigned int  minor,
                    struct class* class) {
  int            errror = 0;
  dev_t          devno;
  struct device* device = NULL;

  /* Allocate the array of devices */
  _keyboard = (struct InputKeyboard*)kzalloc(sizeof(struct InputKeyboard),
                                             GFP_KERNEL);

  if (_keyboard == NULL) {
    errror = -ENOMEM;
    return errror;
  }

  devno = MKDEV(major, minor);

  BUG_ON(_keyboard == NULL || class == NULL);

  /* Memory is to be allocated when the device is opened the first time */
  _keyboard->data        = NULL;
  _keyboard->blockSize   = sizeof(struct input_event);
  _keyboard->buffer_size = _keyboard->blockSize * 1000;
  _keyboard->major       = major;
  _keyboard->class       = class;
  _keyboard->minor       = minor;
  mutex_init(&_keyboard->mutex);

  cdev_init(&_keyboard->cdev, &InputKeyboardFops);
  _keyboard->cdev.owner = THIS_MODULE;

  errror = cdev_add(&_keyboard->cdev, devno, 1);

  if (errror) {
    printk(KERN_ERR "[target] Error %d while trying to add %s (minor %d)",
           errror, LINUX_KEYBOARD_HOOK_WRITER_INPUT_KEYBOARD_DEVICE_NAME,
           minor);
    return errror;
  }

  device = device_create(class, NULL,       /* no parent device */
                         devno, NULL, /* no additional data */
                         LINUX_KEYBOARD_HOOK_WRITER_INPUT_KEYBOARD_DEVICE_NAME);

  if (IS_ERR(device)) {
    errror = PTR_ERR(device);
    printk(
      KERN_ERR "[target] Error %d while trying to create %s (minor %d)",
      errror,
      LINUX_KEYBOARD_HOOK_WRITER_INPUT_KEYBOARD_DEVICE_NAME,
      minor);
    cdev_del(&_keyboard->cdev);
    return errror;
  }

  /* allocate the buffer */
  if (_keyboard->data == NULL) {
    _keyboard->data =
      (unsigned char*)kzalloc(_keyboard->buffer_size, GFP_KERNEL);

    if (_keyboard->data == NULL) {
      printk(KERN_ERR "[target] open(): out of memory\n");
      return -ENOMEM;
    }
  }

  _keyboard->bufferPosition     = 0;
  _keyboard->bufferReadPosition = 0;

  return 0;
}

void
releaseInputKeyboard(void) {
  if (_keyboard == NULL || _keyboard->class == NULL) {
    return;
  }

  device_destroy(_keyboard->class,
                 MKDEV(_keyboard->major, _keyboard->minor));
  cdev_del(&_keyboard->cdev);
  kfree(_keyboard->data);

  return;
}
