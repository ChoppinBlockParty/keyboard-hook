#ifndef MyModule_DeviceInfoBuffer_hpp
#define MyModule_DeviceInfoBuffer_hpp

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/version.h>

#include <asm/uaccess.h>

struct DeviceInfoBuffer {
  unsigned char* data;
  unsigned long  bufferPosition;
  unsigned long  buffer_size;
  unsigned int   major;
  unsigned int   minor;
  struct         class*  class;
  struct mutex   mutex;
  struct cdev    cdev;
};

struct DeviceInfoBuffer*
getDeviceInfoBuffer(void);

int
createDeviceInfoBuffer(unsigned int  major,
                       unsigned int  minor,
                       struct class* class);

void
releaseDeviceInfoBuffer(void);

#endif
