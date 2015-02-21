#ifndef MyModule_InputKeyboard_hpp
#define MyModule_InputKeyboard_hpp

#include "OutputKeyboard.h"

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

struct InputKeyboard {
  unsigned int           major;
  unsigned int           minor;
  struct                 class*  class;
  struct mutex           mutex;
  struct cdev            cdev;
  struct OutputKeyboard* output_device;
};

int
createInputKeyboard(unsigned int           major,
                    unsigned int           minor,
                    struct class*          class,
                    struct OutputKeyboard* output_device);

void
releaseAllInputKeyboard(void);

#endif
