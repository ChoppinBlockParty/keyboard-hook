#include "OutputKeyboard.h"

#include <linux/init.h>
#include <linux/input.h>

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/kthread.h>  // for threads
#include <linux/module.h>
#include <linux/sched.h>  // for task_struct
#include <linux/time.h>
#include <linux/timer.h>

#include "DeviceInfoBuffer.h"
#include "InputKeyboard.h"

struct ListEntry {
  struct list_head      list;
  struct OutputKeyboard device;
};

static LIST_HEAD(_list);

struct ListEntry*
_findListEntry(unsigned int device_number) {
  struct ListEntry* i;

  list_for_each_entry(i, &_list, list) {
    if (i->device.number == device_number) {
      return i;
    }
  }

  return NULL;
}

int
getIntFromData(unsigned char* data, unsigned int* index) {
  unsigned int count = sizeof(int) / sizeof(unsigned char);

  int* result = (int*)&data[*index];

  *index += count;
  return *result;
}

unsigned int
getUnsignedIntFromData(unsigned char* data, unsigned int* index) {
  unsigned int count = sizeof(unsigned int) / sizeof(unsigned char);

  unsigned int* result = (unsigned int*)&data[*index];

  *index += count;
  return *result;
}

void
parseInfo(struct DeviceInfoBuffer* infoBuffer,
          unsigned int*            index,
          struct ListEntry*        entry) {
  unsigned int size = getUnsignedIntFromData(
    infoBuffer->data, index);
  entry->device.number = size;
  size                        = getUnsignedIntFromData(
    infoBuffer->data, index);
  entry->device.device->name = &infoBuffer->data[*index];
  *index                    += size;

  size = getUnsignedIntFromData(
    infoBuffer->data, index);
  entry->device.device->phys = &infoBuffer->data[*index];
  *index                    += size;

  entry->device.device->id.bustype = getIntFromData(infoBuffer->data, index);
  entry->device.device->id.vendor  = getIntFromData(infoBuffer->data, index);
  entry->device.device->id.product = getIntFromData(infoBuffer->data, index);
  entry->device.device->id.version = getIntFromData(infoBuffer->data, index);
}

void
parseCodeBits(struct DeviceInfoBuffer* infoBuffer,
              unsigned int*            index,
              struct ListEntry*        entry) {
  unsigned int type = getUnsignedIntFromData(
    infoBuffer->data, index);
  unsigned int size = getUnsignedIntFromData(
    infoBuffer->data, index);
  unsigned int startIndex = 0;

  entry->device.device->evbit[0] |= BIT_MASK(type);

  startIndex = *index;

  for (; (*index - startIndex) < size;) {
    unsigned int code = getUnsignedIntFromData(infoBuffer->data, index);

    if (type == EV_KEY) {
      entry->device.device->keybit[BIT_WORD(code)] |= BIT_MASK(code);
    } else if (type == EV_LED) {
      entry->device.device->ledbit[BIT_WORD(code)] |= BIT_MASK(code);
    }
  }
}

void
parseDeviceInfo(struct ListEntry* entry) {
  struct DeviceInfoBuffer* infoBuffer = getDeviceInfoBuffer();
  unsigned int             index      = 0;
  unsigned int             size       = 0;
  unsigned int             startIndex = 0;

  parseInfo(infoBuffer, &index, entry);

  size = getUnsignedIntFromData(infoBuffer->data, &index);

  startIndex = index;

  for (; (index - startIndex) < size;)
    parseCodeBits(infoBuffer, &index, entry);
}

int
createOutputKeyboard(unsigned int  major,
                     unsigned int  minor,
                     struct class* class) {
  int               error      = 0;
  struct ListEntry* find_entry = NULL;
  struct ListEntry* entry      = NULL;
  int               size       = 0;

  list_for_each_entry(find_entry, &_list, list) {
    ++size;
  }

  if (size == MAX_NUMBER_OF_DEVICES) {
    printk(KERN_ERR "OutputKeyboard.c: Too many devices allocated already\n");
    return -EFAULT;
  }

  find_entry = NULL;

  entry = (struct ListEntry*)kzalloc(
    sizeof(struct ListEntry),
    GFP_KERNEL);

  if (!entry) {
    printk(KERN_ERR "OutputKeyboard.c: Not enough memory\n");
    error = -ENOMEM;
    return error;
  }

  entry->device.device = input_allocate_device();

  if (!entry->device.device) {
    printk(KERN_ERR "OutputKeyboard.c: Not enough memory\n");
    kfree(entry);
    error = -ENOMEM;
    return error;
  }

  parseDeviceInfo(entry);
  find_entry = _findListEntry(entry->device.number);

  if (find_entry != 0) {
    printk(KERN_ERR "OutputKeyboard.c: Device already exists\n");
    input_free_device(entry->device.device);
    kfree(entry);
    return -EFAULT;
  }

  error = input_register_device(entry->device.device);

  if (error) {
    printk(KERN_ERR "OutputKeyboard.c: Failed to register device\n");
    input_free_device(entry->device.device);
    kfree(entry);
    return error;
  }

  error = createInputKeyboard(major,
                              minor + size,
                              class,
                              &entry->device);

  if (error != 0) {
    printk(KERN_ERR "OutputKeyboard.c: Failed to createInputKeyboard\n");
    input_unregister_device(entry->device.device);
    input_free_device(entry->device.device);
    kfree(entry);
    return error;
  }

  list_add(&entry->list, &_list);

  return 0;
}

void
releaseAllOutputKeyboard(void) {
  struct ListEntry* i;
  list_for_each_entry(i, &_list, list) {
    releaseOutputKeyboard(&i->device);
  }
  releaseAllInputKeyboard();
}

void
releaseOutputKeyboard(struct OutputKeyboard* device) {
  struct ListEntry* entry = _findListEntry(device->number);

  if (entry == 0) {
    return;
  }

  input_unregister_device(entry->device.device);
  input_free_device(entry->device.device);

  list_del(&entry->list);
  kfree(entry);
}
