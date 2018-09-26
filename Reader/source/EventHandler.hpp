#pragma once

#include <linux/input.h>
#include <vector>

typedef std::vector<struct input_event> EventQueue;

extern EventQueue _eventQueue;
extern bool _isEventHandled;

void handleEvent(struct input_event* event, bool useFnAsWindowKey);
