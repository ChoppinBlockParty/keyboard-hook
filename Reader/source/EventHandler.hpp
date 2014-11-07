#ifndef LinuxKeyboardHook_Reader_EventHandle_hpp
#define LinuxKeyboardHook_Reader_EventHandle_hpp

#include <linux/input.h>
#include <vector>

typedef std::vector<struct input_event> EventQueue;

extern EventQueue _eventQueue;

void
handleEvent(struct input_event* event);

#endif
