#ifndef MyModule_OutputKeyboard_hpp
#define MyModule_OutputKeyboard_hpp

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/version.h>

#define MAX_NUMBER_OF_DEVICES 10

struct OutputKeyboard {
  int               number;
  struct input_dev* device;
};

int
createOutputKeyboard(unsigned int  major,
                     unsigned int  minor,
                     struct class* class);

void
releaseAllOutputKeyboard(void);

void
releaseOutputKeyboard(struct OutputKeyboard* device);

#endif
