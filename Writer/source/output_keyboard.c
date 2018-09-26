#include "output_keyboard.h"

#include <linux/init.h>
#include <linux/input.h>

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/kthread.h>  // for threads
#include <linux/module.h>
#include <linux/sched.h>  // for task_struct
#include <linux/time.h>
#include <linux/timer.h>

#include "device_info_buffer.h"
#include "input_keyboard.h"

struct list_entry {
  struct list_head       list;
  struct output_keyboard device;
};

static LIST_HEAD(_list);

struct list_entry*
find_list_entry(unsigned int device_number) {
  struct list_entry* i;

  list_for_each_entry(i, &_list, list) {
    if (i->device.number == device_number) {
      return i;
    }
  }

  return NULL;
}

int
get_int_from_data(unsigned char* data, unsigned int* index) {
  unsigned int count = sizeof(int) / sizeof(unsigned char);

  int* result = (int*)&data[*index];

  *index += count;
  return *result;
}

unsigned int
get_unsigned_int_from_data(unsigned char* data, unsigned int* index) {
  unsigned int count = sizeof(unsigned int) / sizeof(unsigned char);

  unsigned int* result = (unsigned int*)&data[*index];

  *index += count;
  return *result;
}

void parse_info(struct device_info_buffer* infoBuffer,
                unsigned int*            index,
                struct list_entry*       entry) {
  unsigned int size = get_unsigned_int_from_data(infoBuffer->data, index);
  entry->device.number = size;
  size = get_unsigned_int_from_data(infoBuffer->data, index);
  entry->device.device->name = &infoBuffer->data[*index];
  *index += size;

  size = get_unsigned_int_from_data(infoBuffer->data, index);
  entry->device.device->phys = &infoBuffer->data[*index];
  *index += size;

  entry->device.device->id.bustype = get_int_from_data(infoBuffer->data, index);
  entry->device.device->id.vendor  = get_int_from_data(infoBuffer->data, index);
  entry->device.device->id.product = get_int_from_data(infoBuffer->data, index);
  entry->device.device->id.version = get_int_from_data(infoBuffer->data, index);
}

void parse_code_bits(struct device_info_buffer* infoBuffer,
                     unsigned int*            index,
                     struct list_entry*        entry) {
  unsigned int type = get_unsigned_int_from_data(
    infoBuffer->data, index);
  unsigned int size = get_unsigned_int_from_data(
    infoBuffer->data, index);
  unsigned int startIndex = 0;

  entry->device.device->evbit[0] |= BIT_MASK(type);

  startIndex = *index;

  for (; (*index - startIndex) < size;) {
    unsigned int code = get_unsigned_int_from_data(infoBuffer->data, index);

    if (type == EV_KEY) {
      entry->device.device->keybit[BIT_WORD(code)] |= BIT_MASK(code);
    } else if (type == EV_LED) {
      entry->device.device->ledbit[BIT_WORD(code)] |= BIT_MASK(code);
    }
  }
}

void
parse_device_info(struct list_entry* entry) {
  struct device_info_buffer* infoBuffer = get_device_info_buffer();
  unsigned int             index      = 0;
  unsigned int             size       = 0;
  unsigned int             startIndex = 0;

  parse_info(infoBuffer, &index, entry);

  size = get_unsigned_int_from_data(infoBuffer->data, &index);

  startIndex = index;

  for (; (index - startIndex) < size;)
    parse_code_bits(infoBuffer, &index, entry);
}

int create_output_keyboard(unsigned int  major,
                           unsigned int  minor,
                           struct class* class) {
  int               error      = 0;
  struct list_entry* find_entry = NULL;
  struct list_entry* entry      = NULL;
  int               size       = 0;

  list_for_each_entry(find_entry, &_list, list) {
    ++size;
  }

  if (size == MAX_NUMBER_OF_DEVICES) {
    printk(KERN_ERR "output_keyboard.c: Too many devices allocated already\n");
    return -EFAULT;
  }

  find_entry = NULL;

  entry = (struct list_entry*)kzalloc(
    sizeof(struct list_entry),
    GFP_KERNEL);

  if (!entry) {
    printk(KERN_ERR "output_keyboard.c: Not enough memory\n");
    error = -ENOMEM;
    return error;
  }

  entry->device.device = input_allocate_device();

  if (!entry->device.device) {
    printk(KERN_ERR "output_keyboard.c: Not enough memory\n");
    kfree(entry);
    error = -ENOMEM;
    return error;
  }

  parse_device_info(entry);
  find_entry = find_list_entry(entry->device.number);

  if (find_entry != 0) {
    printk(KERN_ERR "output_keyboard.c: Device already exists\n");
    input_free_device(entry->device.device);
    kfree(entry);
    return -EFAULT;
  }

  error = input_register_device(entry->device.device);

  if (error) {
    printk(KERN_ERR "output_keyboard.c: Failed to register device\n");
    input_free_device(entry->device.device);
    kfree(entry);
    return error;
  }

  error = create_input_keyboard(major,
                              minor + size,
                              class,
                              &entry->device);

  if (error != 0) {
    printk(KERN_ERR "output_keyboard.c: Failed to create_input_keyboard\n");
    input_unregister_device(entry->device.device);
    input_free_device(entry->device.device);
    kfree(entry);
    return error;
  }

  list_add(&entry->list, &_list);

  return 0;
}

void
release_all_output_keyboards(void) {
  struct list_entry* i;
  list_for_each_entry(i, &_list, list) {
    release_output_keyboard(&i->device);
  }
  release_all_input_keyboards();
}

void
release_output_keyboard(struct output_keyboard* device) {
  struct list_entry* entry = find_list_entry(device->number);

  if (entry == 0) {
    return;
  }

  input_unregister_device(entry->device.device);
  input_free_device(entry->device.device);

  list_del(&entry->list);
  kfree(entry);
}
