#ifndef _KEYBOARD_HOOK_WRITER_INPUT_KEYBOARD_
#define _KEYBOARD_HOOK_WRITER_INPUT_KEYBOARD_

#include "output_keyboard.h"

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

struct input_keyboard {
  unsigned int            major;
  unsigned int            minor;
  struct class*           class;
  struct mutex            mutex;
  struct cdev             cdev;
  struct output_keyboard* output_device;
};

int
create_input_keyboard(unsigned int            major,
                      unsigned int            minor,
                      struct class*           class,
                      struct output_keyboard* output_device);

void
release_all_input_keyboards(void);

#endif
