#include "InputKeyboard.h"

#include <linux/input.h>
#include <linux/list.h>

#define LINUX_KEYBOARD_HOOK_WRITER_INPUT_KEYBOARD_DEVICE_NAME \
  "LinuxKeyboardHookWriterInputKeyboard"

struct ListEntry {
  struct list_head     list;
  struct InputKeyboard device;
};

static LIST_HEAD(_list);

void _releaseInputKeyboardRoutine(struct ListEntry* entry);

int
_openInputKeyboard(struct inode* inode, struct file* filp) {
  unsigned int          mj     = imajor(inode);
  unsigned int          mn     = iminor(inode);
  struct InputKeyboard* device = NULL;
  struct ListEntry*     entry  = NULL;

  device = container_of(inode->i_cdev, struct InputKeyboard, cdev);
  entry  = container_of(device, struct ListEntry, device);

  if (mj != entry->device.major ||
      mn != entry->device.minor) {
    printk(KERN_ERR "[target] "
                    "No device found with minor=%d and major=%d\n",
           mj, mn);
    return -ENODEV;             /* No such device */
  }

  if (inode->i_cdev != &entry->device.cdev) {
    printk(KERN_ERR "[target] open: internal error\n");
    return -ENODEV;             /* No such device */
  }

  filp->private_data = entry;

  if (mutex_lock_killable(&entry->device.mutex)) {
    printk(KERN_ERR "_openInputKeyboard: Failed to get lock\n");
    return -EINTR;
  }

  return 0;
}

ssize_t
_writeToInputKeyboard(struct file*       filp,
                      const char __user* buf,
                      size_t             count,
                      loff_t*            f_pos) {
  unsigned char data[sizeof(struct input_event)];
  struct input_event* event  = NULL;
  struct ListEntry*   entry  = filp->private_data;
  ssize_t             retval = 0;

  if (count != sizeof(struct input_event)) {
    printk(KERN_ERR "Value size is not equal to buffer block size\n");
    return -EFAULT;
  }

  if (copy_from_user(&data, buf, count) != 0) {
    printk(KERN_ERR "Failed to get data from user\n");
    retval = -EFAULT;
    goto out;
  }

  event = (struct input_event*)data;

  input_event(entry->device.output_device->device,
              event->type,
              event->code,
              event->value);

  retval = count;

out:
  return retval;
}

int
_releaseInputKeyboard(struct inode* inode, struct file* filp) {
  struct ListEntry* entry = filp->private_data;

  releaseOutputKeyboard(entry->device.output_device);

  mutex_unlock(&entry->device.mutex);

  _releaseInputKeyboardRoutine(entry);
  return 0;
}

struct file_operations InputKeyboardFops = {
  .owner   = THIS_MODULE,
  .open    = _openInputKeyboard,
  .write   = _writeToInputKeyboard,
  .release = _releaseInputKeyboard,
};

int
createInputKeyboard(unsigned int           major,
                    unsigned int           minor,
                    struct class*          class,
                    struct OutputKeyboard* output_device) {
  char device_name[256];
  int            errror = 0;
  dev_t          devno;
  struct device* device = NULL;
  struct ListEntry* entry = NULL;

  entry = (struct ListEntry*)kzalloc(sizeof(struct ListEntry), GFP_KERNEL);

  if (entry == NULL) {
    printk(KERN_ERR "InputKeyboard.c: Not enough memory\n");
    errror = -ENOMEM;
    return errror;
  }

  devno = MKDEV(major, minor);

  BUG_ON(class == NULL);

  entry->device.major = major;
  entry->device.class = class;
  entry->device.minor = minor;
  mutex_init(&entry->device.mutex);

  cdev_init(&entry->device.cdev, &InputKeyboardFops);
  entry->device.cdev.owner = THIS_MODULE;

  errror = cdev_add(&entry->device.cdev, devno, 1);

  if (errror) {
    printk(
      KERN_ERR "[target] Error %d while trying to add InputKeyboard (minor %d)",
      errror,
      minor);
    kfree(entry);
    return errror;
  }

  snprintf(device_name, 256, "%s%d",
            LINUX_KEYBOARD_HOOK_WRITER_INPUT_KEYBOARD_DEVICE_NAME,
            output_device->number);

  device = device_create(class, NULL,       /* no parent device */
                         devno, NULL, /* no additional data */
                         device_name);

  if (IS_ERR(device)) {
    errror = PTR_ERR(device);
    printk(
      KERN_ERR "[target] Error %d while trying to create %s (minor %d)",
      errror,
      device_name,
      minor);
    cdev_del(&entry->device.cdev);
    kfree(entry);
    return errror;
  }

  entry->device.output_device = output_device;

  list_add(&entry->list, &_list);

  return 0;
}

void
releaseAllInputKeyboard(void) {
  struct ListEntry* i;
  list_for_each_entry(i, &_list, list) {
    _releaseInputKeyboardRoutine(i);
  }
}

void _releaseInputKeyboardRoutine(struct ListEntry* entry) {
  if (entry->device.class == NULL) {
    return;
  }

  device_destroy(entry->device.class,
                 MKDEV(entry->device.major, entry->device.minor));

  cdev_del(&entry->device.cdev);

  list_del(&entry->list);
  kfree(entry);

  return;
}
