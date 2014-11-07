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

int
createOutputKeyboard(unsigned int  major,
                     unsigned int  minor,
                     struct class* class);

void
releaseOutputKeyboard(void);

#endif
