#ifndef _KEYBOARD_HOOK_WRITER_DEVICE_INFO_BUFFER_
#define _KEYBOARD_HOOK_WRITER_DEVICE_INFO_BUFFER_

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

struct device_info_buffer {
  unsigned char* data;
  unsigned long  bufferPosition;
  unsigned long  buffer_size;
  unsigned int   major;
  unsigned int   minor;
  struct         class*  class;
  struct mutex   mutex;
  struct cdev    cdev;
};

struct device_info_buffer* get_device_info_buffer(void);

int create_device_info_buffer(unsigned int  major,
                              unsigned int  minor,
                              struct class* class);

void destroy_device_info_buffer(void);

#endif
