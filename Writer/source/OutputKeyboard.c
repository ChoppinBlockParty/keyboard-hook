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
#include "handleEvents.h"

static struct input_dev* _keyboard = 0;

static struct task_struct* _loopThread = 0;

int
_runLoop(void* data) {
  struct InputKeyboard* inputKeyboard = getInputKeyboard();
  unsigned int          size          = sizeof(struct input_event);

  while (true) {
    set_current_state(TASK_UNINTERRUPTIBLE);

    if (kthread_should_stop()) {
      break;
    }

    //mutex_lock(&inputKeyboard->mutex);
    // if (mutex_lock_killable(&inputKeyboard->mutex)) {
    //  return -EINTR;
    // }

    while (inputKeyboard->bufferPosition != inputKeyboard->bufferReadPosition) {
      unsigned int       finalSize = inputKeyboard->bufferReadPosition + size;
      struct input_event event;

      if (finalSize >= inputKeyboard->buffer_size) {
        inputKeyboard->bufferReadPosition = 0;
      }

      event = *((struct input_event*)
                &inputKeyboard->data[inputKeyboard->bufferReadPosition]);
      //mutex_unlock(&inputKeyboard->mutex);
      inputKeyboard->bufferReadPosition += size;

      handleEvents(&event);

      input_event(_keyboard,
                  event.type,
                  event.code,
                  event.value);

     // mutex_lock(&inputKeyboard->mutex);
      // if (mutex_lock_killable(&inputKeyboard->mutex)) {
      //  return -EINTR;
      // }
    }

    //mutex_unlock(&inputKeyboard->mutex);
    // msleep(10);
    // Let CPU run other threads, and re scheduled within a specified period of
    // time
    schedule_timeout(HZ / 1000);
  }

  return 0;
}

int
startEventLoop(void) {
  int         err;
  const char* threadName = "Loop Thread";
  _loopThread = kthread_create(_runLoop, NULL, threadName);

  if (IS_ERR(_loopThread)) {
    printk(KERN_ERR "Faile to create loop thread\n");
    err         = PTR_ERR(_loopThread);
    _loopThread = NULL;
    return err;
  }

  wake_up_process(_loopThread);

  return 0;
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
          unsigned int*            index) {
  unsigned int size = getUnsignedIntFromData(
    infoBuffer->data, index);
  _keyboard->name = &infoBuffer->data[*index];
  *index         += size;

  size = getUnsignedIntFromData(
    infoBuffer->data, index);
  _keyboard->phys = &infoBuffer->data[*index];
  *index         += size;

  _keyboard->id.bustype = getIntFromData(infoBuffer->data, index);
  _keyboard->id.vendor  = getIntFromData(infoBuffer->data, index);
  _keyboard->id.product = getIntFromData(infoBuffer->data, index);
  _keyboard->id.version = getIntFromData(infoBuffer->data, index);
}

void
parseCodeBits(struct DeviceInfoBuffer* infoBuffer,
              unsigned int*            index) {
  unsigned int type = getUnsignedIntFromData(
    infoBuffer->data, index);
  unsigned int size = getUnsignedIntFromData(
    infoBuffer->data, index);
  unsigned int startIndex = 0;

  _keyboard->evbit[0] |= BIT_MASK(type);

  startIndex = *index;

  for (; (*index - startIndex) < size;) {
    unsigned int code = getUnsignedIntFromData(infoBuffer->data, index);

    if (type == EV_KEY) {
      _keyboard->keybit[BIT_WORD(code)] |= BIT_MASK(code);
    } else if (type == EV_LED) {
      _keyboard->ledbit[BIT_WORD(code)] |= BIT_MASK(code);
    }
  }
}

void
parseDeviceInfo(void) {
  struct DeviceInfoBuffer* infoBuffer = getDeviceInfoBuffer();
  unsigned int             index      = 0;
  unsigned int             size       = 0;
  unsigned int             startIndex = 0;

  parseInfo(infoBuffer, &index);

  size = getUnsignedIntFromData(infoBuffer->data, &index);
  printk(KERN_WARNING "sdfsdf %d \n", size);

  startIndex = index;

  for (; (index - startIndex) < size;)
    parseCodeBits(infoBuffer, &index);
}

int
createOutputKeyboard(unsigned int  major,
                     unsigned int  minor,
                     struct class* class) {
  int error = 0;

  if (_keyboard) {
    return 0;
  }

  _keyboard = input_allocate_device();

  if (!_keyboard) {
    printk(KERN_ERR "OutputKeyboard.c: Not enough memory\n");
    error = -ENOMEM;
    return error;
  }

  parseDeviceInfo();

  error = input_register_device(_keyboard);

  if (error) {
    printk(KERN_ERR "OutputKeyboard.c: Failed to register device\n");
    input_free_device(_keyboard);
    return error;
  }

  error = createInputKeyboard(major, 1, class);

  if (error != 0) {
    printk(KERN_ERR "OutputKeyboard.c: Failed to createInputKeyboard\n");
    input_free_device(_keyboard);
    return error;
  }

  startEventLoop();

  return 0;
}

void
releaseOutputKeyboard(void) {
  int result;

  if (_loopThread) {
    result = kthread_stop(_loopThread);

    if (!result) {
      printk(KERN_ERR "Failed to stop the loop thread\n");
    }
  }

  if (_keyboard) {
    input_unregister_device(_keyboard);
  }

  releaseInputKeyboard();
}
