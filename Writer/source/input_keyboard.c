#include "input_keyboard.h"

#include <linux/input.h>
#include <linux/list.h>

#define KEYBOARD_HOOK_WRITER_INPUT_KEYBOARD_DEVICE_NAME \
  "keyboard_hook_writer_input_keyboard"

struct list_entry {
  struct list_head     list;
  struct input_keyboard device;
};

static LIST_HEAD(_list);

void release_input_keyboard_routine(struct list_entry* entry);

int open_input_keyboard(struct inode* inode, struct file* filp) {
  unsigned int          mj      = imajor(inode);
  unsigned int          mn      = iminor(inode);
  struct input_keyboard* device = NULL;
  struct list_entry*     entry  = NULL;

  device = container_of(inode->i_cdev, struct input_keyboard, cdev);
  entry  = container_of(device, struct list_entry, device);

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
    printk(KERN_ERR "open_input_keyboard: Failed to get lock\n");
    return -EINTR;
  }

  return 0;
}

ssize_t write_to_input_keyboard(struct file*       filp,
                                const char __user* buf,
                                size_t             count,
                                loff_t*            f_pos) {
  unsigned char data[sizeof(struct input_event)];
  struct input_event* event  = NULL;
  struct list_entry*   entry  = filp->private_data;
  ssize_t             retval = 0;

  if (count != sizeof(struct input_event)) {
    printk(KERN_ERR "Value size is not equal to buffer block size\n");
    return -EFAULT;
  }

  if (raw_copy_from_user(&data, buf, count) != 0) {
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

int release_input_keyboard(struct inode* inode, struct file* filp) {
  struct list_entry* entry = filp->private_data;

  release_output_keyboard(entry->device.output_device);

  mutex_unlock(&entry->device.mutex);

  release_input_keyboard_routine(entry);
  return 0;
}

struct file_operations input_keyboard_Fops = {
  .owner   = THIS_MODULE,
  .open    = open_input_keyboard,
  .write   = write_to_input_keyboard,
  .release = release_input_keyboard,
};

int
create_input_keyboard(unsigned int           major,
                    unsigned int           minor,
                    struct class*          class,
                    struct output_keyboard* output_device) {
  char device_name[256];
  int            error = 0;
  dev_t          devno;
  struct device* device = NULL;
  struct list_entry* entry = NULL;

  entry = (struct list_entry*)kzalloc(sizeof(struct list_entry), GFP_KERNEL);

  if (entry == NULL) {
    printk(KERN_ERR "input_keyboard.c: Not enough memory\n");
    error = -ENOMEM;
    return error;
  }

  devno = MKDEV(major, minor);

  BUG_ON(class == NULL);

  entry->device.major = major;
  entry->device.class = class;
  entry->device.minor = minor;
  mutex_init(&entry->device.mutex);

  cdev_init(&entry->device.cdev, &input_keyboard_Fops);
  entry->device.cdev.owner = THIS_MODULE;

  error = cdev_add(&entry->device.cdev, devno, 1);

  if (error) {
    printk(KERN_ERR "[target] Error %d while trying to add input_keyboard (minor %d)",
           error,
           minor);
    kfree(entry);
    return error;
  }

  snprintf(device_name, 256, "%s%d",
           KEYBOARD_HOOK_WRITER_INPUT_KEYBOARD_DEVICE_NAME,
           output_device->number);

  device = device_create(class, NULL,       /* no parent device */
                         devno, NULL, /* no additional data */
                         device_name);

  if (IS_ERR(device)) {
    error = PTR_ERR(device);
    printk(
      KERN_ERR "[target] Error %d while trying to create %s (minor %d)",
      error,
      device_name,
      minor);
    cdev_del(&entry->device.cdev);
    kfree(entry);
    return error;
  }

  entry->device.output_device = output_device;

  list_add(&entry->list, &_list);

  return 0;
}

void
release_all_input_keyboards(void) {
  struct list_entry* i;
  list_for_each_entry(i, &_list, list) {
    release_input_keyboard_routine(i);
  }
}

void release_input_keyboard_routine(struct list_entry* entry) {
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
