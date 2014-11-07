#include "handleEvents.h"

unsigned int germanShiftCode = 86;
unsigned int leftShift       = 86;

void
handleEvents(struct input_event* event) {
  if (event->code == germanShiftCode) {
    event->code = 42;
  }
}
