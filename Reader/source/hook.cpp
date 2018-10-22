#include "hook.hpp"

#include <fcntl.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "EventHandler.hpp"

#define KEYBOARD_HOOK_WRITER_INPUT_KEYBOARD_DEVICE_MASTER "/dev/input/event"

#define KEYBOARD_HOOK_WRITER_DEVICE_INFO_BUFFER_DEVICE_PATH                              \
  "/dev/keyboard_hook_writer_device_info_buffer"

#define KEYBOARD_HOOK_WRITER_INPUT_KEYBOARD_DEVICE_PATH                                  \
  "/dev/keyboard_hook_writer_input_keyboard"

EventQueue _eventQueue;
bool _isEventHandled;

class SimpleKey {
public:
  SimpleKey(unsigned int keyCode) : _keyCode(keyCode) {}

  unsigned int& keyCode() { return _keyCode; }

private:
  unsigned int _keyCode;
};

SimpleKey capsLock(66);
SimpleKey germanShift(94);
SimpleKey semiColon(66);

void logLog(char const* msg, char const* format, va_list args) {
  fprintf(stdout, "[%s] ", msg);
  vfprintf(stdout, format, args);
  fprintf(stdout, "\n");
}

void logInfo(char const* format, ...) {
  va_list args;
  va_start(args, format);
  logLog("INFO", format, args);
  va_end(args);
}

void log_warn(char const* format, ...) {
  va_list args;
  va_start(args, format);
  logLog("WARN", format, args);
  va_end(args);
}

void logError(char const* format, ...) {
  va_list args;
  va_start(args, format);
  logLog("ERROR", format, args);
  va_end(args);
}

typedef std::vector<unsigned char> Buffer;

Buffer deviceInfo;

void writeToBuffer(Buffer* buffer, unsigned int value) {
  unsigned char* pointer = (unsigned char*)(&value);
  int count = sizeof(unsigned int) / sizeof(unsigned char);

  for (int i = 0; i < count; ++i)
    buffer->push_back(pointer[i]);
}

void writeToBuffer(Buffer* buffer, int value) {
  unsigned char* pointer = (unsigned char*)(&value);
  int count = sizeof(int) / sizeof(unsigned char);

  for (int i = 0; i < count; ++i)
    buffer->push_back(pointer[i]);
}

void writeToBuffer(Buffer* buffer, std::string const* value) {
  for (unsigned int i = 0; i < value->size(); ++i)
    buffer->push_back(value->at(i));

  buffer->push_back('\0');
}

void writeToBuffer(Buffer* buffer, Buffer const* value) {
  for (unsigned int i = 0; i < value->size(); ++i)
    buffer->push_back(value->at(i));
}

static void print_abs_bits(struct libevdev* dev, int axis) {
  const struct input_absinfo* abs;

  if (!libevdev_has_event_code(dev, EV_ABS, axis)) {
    return;
  }

  abs = libevdev_get_abs_info(dev, axis);

  printf("	Value	%6d\n", abs->value);
  printf("	Min	%6d\n", abs->minimum);
  printf("	Max	%6d\n", abs->maximum);

  if (abs->fuzz) {
    printf("	Fuzz	%6d\n", abs->fuzz);
  }

  if (abs->flat) {
    printf("	Flat	%6d\n", abs->flat);
  }

  if (abs->resolution) {
    printf("	Resolution	%6d\n", abs->resolution);
  }
}

static void print_code_bits(struct libevdev* dev, unsigned int type, unsigned int max) {
  unsigned int i;

  for (i = 0; i <= max; i++) {
    if (!libevdev_has_event_code(dev, type, i)) {
      continue;
    }

    printf("    Event code %i (%s)\n", i, libevdev_event_code_get_name(type, i));

    if (type == EV_ABS) {
      print_abs_bits(dev, i);
    }
  }
}

static void print_bits(struct libevdev* dev) {
  unsigned int i;
  printf("Supported events:\n");

  for (i = 0; i <= EV_MAX; i++) {
    if (libevdev_has_event_type(dev, i)) {
      printf("  Event type %d (%s)\n", i, libevdev_event_type_get_name(i));
    }

    switch (i) {
    case EV_KEY:
      print_code_bits(dev, EV_KEY, KEY_MAX);
      break;

    case EV_REL:
      print_code_bits(dev, EV_REL, REL_MAX);
      break;

    case EV_ABS:
      print_code_bits(dev, EV_ABS, ABS_MAX);
      break;

    case EV_LED:
      print_code_bits(dev, EV_LED, LED_MAX);
      break;
    }
  }
}

void writeCodeBits(struct libevdev* dev,
                   unsigned int type,
                   unsigned int max,
                   Buffer* buffer) {
  unsigned int i;

  for (i = 0; i <= max; i++) {
    if (!libevdev_has_event_code(dev, type, i)) {
      continue;
    }

    writeToBuffer(buffer, i);
  }
}

void gatherInfo(unsigned const& number, struct libevdev* dev) {
  std::string deviceName("");
  const char* deviceNameChars = libevdev_get_name(dev);

  if (deviceNameChars != 0) {
    deviceName = deviceNameChars;
  }

  std::string devicePhys("");
  const char* devicePhysChars = libevdev_get_phys(dev);

  if (devicePhysChars != 0) {
    devicePhys = devicePhysChars;
  }

  const int deviceIdBusType = libevdev_get_id_bustype(dev);

  const int deviceIdVendor = libevdev_get_id_vendor(dev);

  const int deviceIdProduct = libevdev_get_id_product(dev);

  const int deviceIdVersion = libevdev_get_id_version(dev);

  writeToBuffer(&deviceInfo, (unsigned int)number);
  writeToBuffer(&deviceInfo, (unsigned int)deviceName.size() + 1);
  writeToBuffer(&deviceInfo, &deviceName);
  writeToBuffer(&deviceInfo, (unsigned int)devicePhys.size() + 1);
  writeToBuffer(&deviceInfo, &devicePhys);
  writeToBuffer(&deviceInfo, deviceIdBusType);
  writeToBuffer(&deviceInfo, deviceIdVendor);
  writeToBuffer(&deviceInfo, deviceIdProduct);
  writeToBuffer(&deviceInfo, deviceIdVersion);
}

void gatherEvents(struct libevdev* dev) {
  unsigned int i;

  std::vector<Buffer*> buffers;

  for (i = 0; i <= EV_MAX; i++) {
    if (!libevdev_has_event_type(dev, i)) {
      continue;
    }

    Buffer* buffer = new Buffer();

    switch (i) {
    case EV_KEY:
      writeCodeBits(dev, EV_KEY, KEY_MAX, buffer);
      break;

    case EV_REL:
      writeCodeBits(dev, EV_REL, REL_MAX, buffer);
      break;

    case EV_ABS:
      writeCodeBits(dev, EV_ABS, ABS_MAX, buffer);
      break;

    case EV_LED:
      writeCodeBits(dev, EV_LED, LED_MAX, buffer);
      break;
    }

    unsigned int size = buffer->size();
    Buffer* newBuffer = new Buffer();
    writeToBuffer(newBuffer, i);
    writeToBuffer(newBuffer, size);
    writeToBuffer(newBuffer, buffer);
    buffers.push_back(newBuffer);
    delete buffer;
  }

  unsigned int size = 0;

  for (auto& buffer : buffers)
    size += buffer->size();

  writeToBuffer(&deviceInfo, size);

  for (auto& buffer : buffers)
    writeToBuffer(&deviceInfo, buffer);

  for (auto& buffer : buffers)
    delete buffer;
}

static void print_props(struct libevdev* dev) {
  unsigned int i;
  printf("Properties:\n");

  for (i = 0; i <= INPUT_PROP_MAX; i++)
    if (libevdev_has_property(dev, i)) {
      printf("  Property type %d (%s)\n", i, libevdev_property_get_name(i));
    }
}

static int print_event(struct input_event* event) {
  if (event->type == EV_SYN) {
    printf("Event: time %ld.%06ld, ++++++++++++++++++++ %s +++++++++++++++\n",
           event->time.tv_sec,
           event->time.tv_usec,
           libevdev_event_type_get_name(event->type));
  } else {
    printf("Event: time %ld.%06ld, type %d (%s), code %d (%s), value %d\n",
           event->time.tv_sec,
           event->time.tv_usec,
           event->type,
           libevdev_event_type_get_name(event->type),
           event->code,
           libevdev_event_code_get_name(event->type, event->code),
           event->value);
  }

  return 0;
}

static int print_sync_event(struct input_event* event) {
  printf("SYNC: ");
  print_event(event);

  return 0;
}

const char* outputDeviceName1 = KEYBOARD_HOOK_WRITER_DEVICE_INFO_BUFFER_DEVICE_PATH;

const char* outputDeviceName2 = KEYBOARD_HOOK_WRITER_INPUT_KEYBOARD_DEVICE_PATH;

int outpuDeviceFileDescriptor1 = -1;
int outpuDeviceFileDescriptor2 = -1;

bool openOutputDevice() {
  outpuDeviceFileDescriptor1 = open(outputDeviceName1, O_WRONLY | O_SYNC);

  if (outpuDeviceFileDescriptor1 <= 0) {
    logError("Failed to open %s", outputDeviceName1);

    return false;
  }

  return true;
}

int sendDeviceInfo(unsigned const& device_number) {
  int result = write(
    outpuDeviceFileDescriptor1, deviceInfo.data(), sizeof(char) * deviceInfo.size());

  if (result < 0) {
    logError("Failed to write the device info");

    return result;
  }

  result = close(outpuDeviceFileDescriptor1);

  if (result < 0) {
    logError("Failed to submit the device info");

    return result;
  }

  std::string output_device_name(outputDeviceName2);
  output_device_name += std::to_string(device_number);

  outpuDeviceFileDescriptor2 = open(output_device_name.c_str(), O_WRONLY);

  if (outpuDeviceFileDescriptor2 <= 0) {
    logError("Failed to open %s", output_device_name.c_str());

    return -1;
  }

  return 0;
}

int writeEvent(struct input_event* event) {
  int result
    = write(outpuDeviceFileDescriptor2, (void*)event, sizeof(struct input_event));

  if (result == 0) {
    logInfo("0 was written even %ld %ld", (void*)event, sizeof(struct input_event));
  }

  if (result < 0) {
    logError("Failed to write an event");

    return result;
  }

  return 0;
}

bool _isInputDeviceGrabbed = false;

int sendEvent(struct input_event* event, bool useFnAsWindowKey) {
  if (!_isInputDeviceGrabbed) {
    return 0;
  }

  handleEvent(event, useFnAsWindowKey);

  int result = 0;

  if (_isEventHandled) {
    int i = 0;

    for (auto& queueEvent : _eventQueue) {
      i++;
      result = writeEvent(&queueEvent);

      if (result != 0) {
        break;
      }
    }

    _eventQueue.clear();
    _isEventHandled = false;
  } else {
    result = writeEvent(event);
  }

  return result;
}

void viewDevices() {
  for (int i = 0; i < 32; ++i) {
    std::string devicePath = "/dev/input/event" + std::to_string(i);
    struct libevdev* dev = NULL;
    int fd;
    fd = open(devicePath.data(), O_RDONLY | O_NONBLOCK);

    int err;
    dev = libevdev_new();

    if (!dev) {
      continue;
    }

    err = libevdev_set_fd(dev, fd);

    if (err < 0) {
      // printf("Failed (errno %d): %s\n", -err, strerror(-err));
      continue;
    }

    std::string deviceName(libevdev_get_name(dev));

    std::string deviceUId("");
    const char* deviceUIdChars = libevdev_get_uniq(dev);

    if (deviceUIdChars != 0) {
      deviceUId = deviceUIdChars;
    }

    std::string devicePhys("");
    const char* devicePhysChars = libevdev_get_phys(dev);

    if (devicePhysChars != 0) {
      devicePhys = devicePhysChars;
    }

    deviceName = devicePath + " | " + deviceName + " | " + deviceUId + " | " + devicePhys;

    logInfo(deviceName.data());

    libevdev_free(dev);
  }
}

void viewEventsPure(std::string const& devicePath) {
  int fd;
  fd = open(devicePath.c_str(), O_RDONLY);

  if (fd <= 0) {
    logError("Failed to open a device file descriptor");
  }

  unsigned int const size = sizeof(struct input_event);

  void* buffer = new unsigned char[size];

  while (true) {
    ssize_t resultSize = read(fd, buffer, size);

    if (resultSize == 0) {
      continue;
    }

    struct input_event* event = (struct input_event*)buffer;

    if (event->code == 0 && event->type == 0 && event->value == 0) {
      // continue;
    }

    print_event(event);
  }
}

void viewEvents(std::string const& devicePath) {
  struct libevdev* dev = NULL;
  int fd;
  fd = open(devicePath.c_str(), O_RDONLY | O_NONBLOCK);

  int err;
  dev = libevdev_new();

  if (!dev) {
    return;
  }

  err = libevdev_set_fd(dev, fd);

  if (err < 0) {
    printf("Failed in viewEvents (errno %d): %s\n", -err, strerror(-err));

    return;
  }

  print_bits(dev);
  print_props(dev);

  int rc = 0;

  do {
    struct input_event event;

    rc = libevdev_next_event(
      dev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING, &event);

    if (rc == LIBEVDEV_READ_STATUS_SYNC) {
      printf("::::::::::::::::::::: dropped ::::::::::::::::::::::\n");

      while (rc == LIBEVDEV_READ_STATUS_SYNC) {
        print_sync_event(&event);
        rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_SYNC, &event);
      }

      printf("::::::::::::::::::::: re-synced ::::::::::::::::::::::\n");
    } else if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
      print_event(&event);
    }
  } while (rc == LIBEVDEV_READ_STATUS_SYNC || rc == LIBEVDEV_READ_STATUS_SUCCESS
           || rc == -EAGAIN);

  if (rc != LIBEVDEV_READ_STATUS_SUCCESS && rc != -EAGAIN) {
    fprintf(stderr, "Failed to handle events: %s\n", strerror(-rc));
  }

  rc = 0;

  libevdev_free(dev);
}

struct libevdev* InputDevice = NULL;

void releaseDevices() {
  if (outpuDeviceFileDescriptor2 > 0) {
    close(outpuDeviceFileDescriptor2);
  }

  // libevdev_grab(InputDevice, LIBEVDEV_UNGRAB);
  libevdev_free(InputDevice);
}

int grabInputDevice() {
  if (_isInputDeviceGrabbed) {
    return 0;
  }

  int err = libevdev_grab(InputDevice, LIBEVDEV_GRAB);

  if (err != 0) {
    logError("Failed to grab input device");

    return err;
  }

  _isInputDeviceGrabbed = true;

  return 0;
}

void initializeAndRunForwarding(unsigned device_number, bool useFnAsWindowKey) {
  gatherInfo(device_number, InputDevice);
  gatherEvents(InputDevice);

  if (!openOutputDevice()) {
    return;
  }

  if (sendDeviceInfo(device_number) != 0) {
    return;
  }

  // viewDevices();
  // std::thread thread2(viewEvents);

  int rc = 0;

  do {
    struct input_event event;
    rc = libevdev_next_event(
      InputDevice, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING, &event);

    if (event.type == EV_SYN) {
      if (grabInputDevice() != 0) {
        return;
      }
    }

    if (rc == LIBEVDEV_READ_STATUS_SYNC) {
      while (rc == LIBEVDEV_READ_STATUS_SYNC) {
        if (sendEvent(&event, useFnAsWindowKey) != 0) {
          return;
        }

        rc = libevdev_next_event(InputDevice, LIBEVDEV_READ_FLAG_SYNC, &event);
      }

      if (rc == -EAGAIN) {
        continue;
      }

      if (sendEvent(&event, useFnAsWindowKey) != 0) {
        return;
      }
    } else if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
      if (sendEvent(&event, useFnAsWindowKey) != 0) {
        return;
      }
    }
  } while (rc == LIBEVDEV_READ_STATUS_SYNC || rc == LIBEVDEV_READ_STATUS_SUCCESS
           || rc == -EAGAIN);

  if (rc != LIBEVDEV_READ_STATUS_SUCCESS && rc != -EAGAIN) {
    fprintf(stderr, "Failed to handle events: %s\n", strerror(-rc));
  }

  // thread2.join();
}

void handleEvents(unsigned device_number,
                  std::string const& devicePath,
                  bool useFnAsWindowKey) {
  int fd;
  fd = open(devicePath.c_str(), O_RDONLY);

  int err;
  InputDevice = libevdev_new();

  if (!InputDevice) {
    return;
  }

  err = libevdev_set_fd(InputDevice, fd);

  if (err < 0) {
    printf("Failed to open input device (errno %d): %s\n", -err, strerror(-err));

    return;
  }

  initializeAndRunForwarding(device_number, useFnAsWindowKey);

  releaseDevices();
}

void setupHook(int device, bool doShowEvent, bool useFnAsWindowKey) {
  // For some reason it is required otherwise you will get empty (0) events at
  // the first run
  _eventQueue.reserve(9);
  _isEventHandled = false;

  if (device < 0) {
    viewDevices();
  } else {
    std::string devicePath = KEYBOARD_HOOK_WRITER_INPUT_KEYBOARD_DEVICE_MASTER;
    devicePath += std::to_string(device);

    if (doShowEvent) {
      viewEvents(devicePath);
    } else {
      handleEvents(device, devicePath, useFnAsWindowKey);
    }
  }

  // std::thread thread(viewEvents);
}
