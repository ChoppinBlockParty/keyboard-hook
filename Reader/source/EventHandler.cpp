#include "EventHandler.hpp"

#include <cstdio>

typedef unsigned int KeyCode;

struct input_event*
createEvent() {
  _eventQueue.emplace_back();
  _isEventHandled = true;

  return &_eventQueue.back();
}

void
sendKeyEvent(struct timeval* time,
             int const&      isPressed,
             KeyCode const&  code) {
  auto mscEvent  = createEvent();
  auto event     = createEvent();
  auto syncEvent = createEvent();

  mscEvent->time  = *time;
  mscEvent->type  = EV_MSC;
  mscEvent->code  = MSC_SCAN;
  mscEvent->value = code;

  event->time  = *time;
  event->type  = EV_KEY;
  event->code  = code;
  event->value = isPressed;

  syncEvent->time  = *time;
  syncEvent->type  = EV_SYN;
  syncEvent->code  = SYN_REPORT;
  syncEvent->value = 0;
}

namespace LinuxKeyboardHook {
namespace Reader {
class Key {
public:
  Key(KeyCode const& code) : _code(code) {}

  Key(Key const& other) : _code(other._code) {}

  virtual
  ~Key() {}

  Key&
  operator=(Key const& other) {
    _code = other._code;

    return *this;
  }

  KeyCode&
  code() { return _code; }

  virtual
  void
  press(struct timeval* time) {
    sendKeyEvent(time, 1, _code);
  }

  virtual
  void
  release(struct timeval* time) {
    sendKeyEvent(time, 0, _code);
  }

private:
  KeyCode _code;
};

class Modifier : public Key {
public:
  typedef Key Base;

  Modifier(KeyCode const& code) : Base(code),
                                  _isPressed(false) {}

  Modifier(Modifier const& other) : Base(other),
                                    _isPressed(other._isPressed) {}

  virtual
  ~Modifier() {}

  Modifier&
  operator=(Modifier const& other) {
    this->Base::operator=(other);
    _isPressed = other._isPressed;

    return *this;
  }

  bool&
  isPressed() { return _isPressed; }

  void
  saveState() {
    _previousState = _isPressed;
  }

  virtual
  void
  press(struct timeval* time) {
    if (!_isPressed) {
      this->Base::press(time);
      _isPressed = true;
    }
  }

  virtual
  void
  release(struct timeval* time) {
    if (_isPressed) {
      this->Base::release(time);
      _isPressed = false;
    }
  }

  void
  restoreState(struct timeval* time) {
    if (_previousState != _isPressed) {
      if (_previousState) {
        press(time);
      } else {
        release(time);
      }
    }
  }

private:
  bool _isPressed;
  bool _previousState;
};
}
}

static LinuxKeyboardHook::Reader::Modifier _altSemicolon(39);
static LinuxKeyboardHook::Reader::Modifier _semicolonLeftAlt(56);
static LinuxKeyboardHook::Reader::Modifier _semicolonRightAlt(100);
static LinuxKeyboardHook::Reader::Modifier _semicolonLeftCtrl(29);
static LinuxKeyboardHook::Reader::Modifier _semicolon(39);
static LinuxKeyboardHook::Reader::Modifier _fakeBackspace(14);

static LinuxKeyboardHook::Reader::Modifier _escape(1);
static LinuxKeyboardHook::Reader::Modifier _capslock(58);
static LinuxKeyboardHook::Reader::Modifier _germanShift(86);
static LinuxKeyboardHook::Reader::Modifier _leftShift(42);
static LinuxKeyboardHook::Reader::Modifier _rightShift(54);
static LinuxKeyboardHook::Reader::Modifier _leftAlt(56);
static LinuxKeyboardHook::Reader::Modifier _rightAlt(100);
static LinuxKeyboardHook::Reader::Modifier _leftCtrl(29);
static LinuxKeyboardHook::Reader::Modifier _rightCtrl(97);

bool
isShiftPressed() {
  return _leftShift.isPressed() ||
         _rightShift.isPressed() ||
         _germanShift.isPressed();
}

bool
isAltPressed() {
  return _leftAlt.isPressed() ||
         _rightAlt.isPressed();
}

bool
handleCapsLock(struct input_event* event) {
  if (event->type == EV_KEY &&
      event->code == _capslock.code()) {
    event->code = _escape.code();

    if (event->value == 1) {
      _capslock.isPressed() = true;
    } else if (event->value == 0) {
      _capslock.isPressed() = false;
    }

    return true;
  }

  return false;
}

bool
handleGermanShift(struct input_event* event) {
  if (event->type == EV_KEY &&
      event->code == _germanShift.code()) {
    event->code = _leftShift.code();

    if (event->value == 1) {
      _germanShift.isPressed() = true;
    } else if (event->value == 0) {
      _germanShift.isPressed() = false;
    }

    return true;
  }

  return false;
}

bool
handleAlt(struct input_event* event) {
  if (event->type != EV_KEY) {
    return false;
  }

  if (event->code == _leftAlt.code()) {
    if (event->value == 1) {
      _leftAlt.isPressed() = true;
    } else if (event->value == 0) {
      _leftAlt.isPressed() = false;
    }

    if (_altSemicolon.isPressed()) {
      _isEventHandled = true;
    }

    return true;
  } else if (event->code == _rightAlt.code()) {
    if (event->value == 1) {
      _rightAlt.isPressed() = true;
    } else if (event->value == 0) {
      _rightAlt.isPressed() = false;
    }

    if (_altSemicolon.isPressed()) {
      _isEventHandled = true;
    }

    return true;
  }

  return false;
}

bool
handleCtrl(struct input_event* event) {
  if (event->type != EV_KEY) {
    return false;
  }

  if (event->code == _leftCtrl.code()) {
    if (event->value == 1) {
      _leftCtrl.isPressed() = true;
    } else if (event->value == 0) {
      _leftCtrl.isPressed() = false;
    }

    return true;
  } else if (event->code == _rightCtrl.code()) {
    if (event->value == 1) {
      _rightCtrl.isPressed() = true;
    } else if (event->value == 0) {
      _rightCtrl.isPressed() = false;
    }

    return true;
  }

  return false;
}


bool
handleShift(struct input_event* event) {
  if (event->type != EV_KEY) {
    return false;
  }

  if (event->code == _leftShift.code()) {
    if (event->value == 1) {
      _leftShift.isPressed() = true;
    } else if (event->value == 0) {
      _leftShift.isPressed() = false;
    }

    return true;
  } else if (event->code == _rightShift.code()) {
    if (event->value == 1) {
      _rightShift.isPressed() = true;
    } else if (event->value == 0) {
      _rightShift.isPressed() = false;
    }

    return true;
  }

  return false;
}

bool
handleSemicolon(struct input_event* event) {
  if (event->type != EV_KEY ||
      event->code != _semicolon.code()) {
    return false;
  }

  if (_semicolon.isPressed()) {
    if (!isShiftPressed()) {
      _leftShift.press(&event->time);

      sendKeyEvent(&event->time,
                   event->value,
                   event->code);
      _leftShift.release(&event->time);
    }

    if (event->value == 1) {
      _semicolon.isPressed() = true;
    } else if (event->value == 0) {
      _semicolon.isPressed() = false;
    }

    return true;
  }

  if (_altSemicolon.isPressed()) {
      sendKeyEvent(&event->time,
                 event->value,
                 event->code);

    if (event->value == 1) {
      _altSemicolon.isPressed() = true;
    } else if (event->value == 0) {
      _altSemicolon.isPressed() = false;

      //if (_leftAlt.isPressed()) {
      //  _semicolonLeftAlt.restoreState(&event->time);
      //}

      //if (_rightAlt.isPressed()) {
      //  _semicolonRightAlt.restoreState(&event->time);
      //}
    }

    return true;
  }

  if (_fakeBackspace.isPressed()) {
    if (event->value == 1) {
      _fakeBackspace.isPressed() = true;
    } else if (event->value == 0) {
      _fakeBackspace.isPressed() = false;
    }
    event->code = _fakeBackspace.code();

    return true;
  }

  if (isShiftPressed()) {
    if (event->value == 1) {
      _semicolon.isPressed() = true;
    } else if (event->value == 0) {
      _semicolon.isPressed() = false;
    }
    //
  } else if (isAltPressed()) {
    _semicolonLeftCtrl.isPressed() = _leftCtrl.isPressed();
    _semicolonLeftCtrl.saveState();
    _semicolonLeftCtrl.isPressed() = false;
    _semicolonLeftCtrl.press(&event->time);

    _semicolonLeftAlt.isPressed()  = _leftAlt.isPressed();
    _semicolonRightAlt.isPressed() = _rightAlt.isPressed();
    _semicolonLeftAlt.saveState();
    _semicolonLeftAlt.release(&event->time);
    _semicolonRightAlt.saveState();
    _semicolonRightAlt.release(&event->time);

    _semicolonLeftCtrl.release(&event->time);
    _semicolonLeftCtrl.restoreState(&event->time);

      sendKeyEvent(&event->time,
                 event->value,
                 event->code);

    // _leftAlt.restoreState(&event->time);
    // _rightAlt.restoreState(&event->time);

    if (event->value == 1) {
      _altSemicolon.isPressed() = true;
    } else if (event->value == 0) {
      _altSemicolon.isPressed() = false;
    }
  } else {
    event->code = _fakeBackspace.code();

    if (event->value == 1) {
      _fakeBackspace.isPressed() = true;
    } else if (event->value == 0) {
      _fakeBackspace.isPressed() = false;
    }
  }

  return true;
}

void
handleEvent(struct input_event* event) {
  if (handleCapsLock(event)) {
    //
  } else if (handleGermanShift(event)) {
    //
  } else if (handleShift(event)) {
    //
  } else if (handleAlt(event)) {
    //
  } else if (handleCtrl(event)) {
    //
  } else if (handleSemicolon(event)) {
    //
  }
}
