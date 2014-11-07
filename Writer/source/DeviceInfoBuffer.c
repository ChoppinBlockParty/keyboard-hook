#include "DeviceInfoBuffer.h"

#include "OutputKeyboard.h"

#define DEVICE_INFO_BUFFER_NAME "mymodule0"

struct DeviceInfoBuffer* _deviceInfoBuffer;

struct DeviceInfoBuffer*
getDeviceInfoBuffer(void) {
  return _deviceInfoBuffer;
}

int
_openDeviceInfoBuffer(struct inode* inode, struct file* filp) {
  unsigned int mj = imajor(inode);
  unsigned int mn = iminor(inode);

  if (mj != _deviceInfoBuffer->major ||
      mn != _deviceInfoBuffer->minor) {
    printk(KERN_WARNING "[target] "
                        "No device found with minor=%d and major=%d\n",
           mj, mn);
    return -ENODEV;             /* No such device */
  }

  if (inode->i_cdev != &_deviceInfoBuffer->cdev) {
    printk(KERN_WARNING "[target] open: internal error\n");
    return -ENODEV;             /* No such device */
  }

  return 0;
}

ssize_t
_writeToDeviceInfoBuffer(struct file*       filp,
                         const char __user* buf,
                         size_t             count,
                         loff_t*            f_pos) {
  ssize_t retval = 0;

  if (mutex_lock_killable(&_deviceInfoBuffer->mutex)) {
    return -EINTR;
  }

  if (_deviceInfoBuffer->bufferPosition >= _deviceInfoBuffer->buffer_size) {
    /* Writing beyond the end of the buffer is not allowed. */
    retval = -EINVAL;
    goto out;
  }

  if ((_deviceInfoBuffer->bufferPosition + count) >=
      _deviceInfoBuffer->buffer_size) {
    count = _deviceInfoBuffer->buffer_size - _deviceInfoBuffer->bufferPosition;
  }

  if (copy_from_user(&(_deviceInfoBuffer->data[
                         _deviceInfoBuffer->bufferPosition]),
                     buf, count) != 0) {
    retval = -EFAULT;
    goto out;
  }

  _deviceInfoBuffer->bufferPosition += count;
  *f_pos                             = _deviceInfoBuffer->bufferPosition;
  retval                             = count;

out:
  mutex_unlock(&_deviceInfoBuffer->mutex);
  return retval;
}

int
_releaseDeviceInfoBuffer(struct inode* inode, struct file* filp) {
  int err = createOutputKeyboard(_deviceInfoBuffer->major,
                                 _deviceInfoBuffer->minor,
                                 _deviceInfoBuffer->class);
  if (err != 0) {
    printk(KERN_WARNING "Failed to createOutputKeyboard");
    return err;
  }

  return 0;
}

struct file_operations DeviceInfoBufferFops = {
  .owner   = THIS_MODULE,
  .open    = _openDeviceInfoBuffer,
  .write   = _writeToDeviceInfoBuffer,
  .release = _releaseDeviceInfoBuffer,
};

int
createDeviceInfoBuffer(unsigned int  major,
                       unsigned int  minor,
                       struct class* class) {
  int            errror = 0;
  dev_t          devno;
  struct device* device = NULL;

  /* Allocate the array of devices */
  _deviceInfoBuffer = (struct DeviceInfoBuffer*)kzalloc(
    sizeof(struct DeviceInfoBuffer),
    GFP_KERNEL);

  if (_deviceInfoBuffer == NULL) {
    errror = -ENOMEM;
    return errror;
  }

  devno = MKDEV(major, minor);

  BUG_ON(_deviceInfoBuffer == NULL || class == NULL);

  /* Memory is to be allocated when the device is opened the first time */
  _deviceInfoBuffer->data        = NULL;
  _deviceInfoBuffer->buffer_size = 40000;
  _deviceInfoBuffer->major       = major;
  _deviceInfoBuffer->class       = class;
  _deviceInfoBuffer->minor       = minor;
  mutex_init(&_deviceInfoBuffer->mutex);

  cdev_init(&_deviceInfoBuffer->cdev, &DeviceInfoBufferFops);
  _deviceInfoBuffer->cdev.owner = THIS_MODULE;

  errror = cdev_add(&_deviceInfoBuffer->cdev, devno, 1);

  if (errror) {
    printk(KERN_WARNING "[target] Error %d while trying to add %s (minor %d)",
           errror, DEVICE_INFO_BUFFER_NAME, minor);
    return errror;
  }

  device = device_create(class, NULL,       /* no parent device */
                         devno, NULL, /* no additional data */
                         DEVICE_INFO_BUFFER_NAME);

  if (IS_ERR(device)) {
    errror = PTR_ERR(device);
    printk(
      KERN_WARNING "[target] Error %d while trying to create %s (minor %d)",
      errror,
      DEVICE_INFO_BUFFER_NAME,
      minor);
    cdev_del(&_deviceInfoBuffer->cdev);
    return errror;
  }

  /* allocate the buffer */
  if (_deviceInfoBuffer->data == NULL) {
    _deviceInfoBuffer->data =
      (unsigned char*)kzalloc(_deviceInfoBuffer->buffer_size, GFP_KERNEL);

    if (_deviceInfoBuffer->data == NULL) {
      printk(KERN_WARNING "[target] open(): out of memory\n");
      return -ENOMEM;
    }
  }

  _deviceInfoBuffer->bufferPosition = 0;

  return 0;
}

void
releaseDeviceInfoBuffer(void) {
  BUG_ON(_deviceInfoBuffer == NULL || _deviceInfoBuffer->class == NULL);
  releaseOutputKeyboard();
  device_destroy(_deviceInfoBuffer->class,
                 MKDEV(_deviceInfoBuffer->major, _deviceInfoBuffer->minor));
  cdev_del(&_deviceInfoBuffer->cdev);
  kfree(_deviceInfoBuffer->data);
  return;
}
