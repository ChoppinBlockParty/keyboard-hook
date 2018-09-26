#ifndef _KEYBOARD_HOOK_WRITER_OUTPUT_KEYBOARD_
#define _KEYBOARD_HOOK_WRITER_OUTPUT_KEYBOARD_

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

struct output_keyboard {
  int               number;
  struct input_dev* device;
};

int
create_output_keyboard(unsigned int  major,
                       unsigned int  minor,
                       struct class* class);

void
release_all_output_keyboards(void);

void
release_output_keyboard(struct output_keyboard* device);
#endif
