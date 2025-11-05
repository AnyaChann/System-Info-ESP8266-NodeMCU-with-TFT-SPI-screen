/*
 * Button Handler Module
 * Xử lý button press với debounce và edge detection
 */

#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>

class ButtonHandler {
private:
  uint8_t pin;
  bool lastState;
  unsigned long lastPressTime;
  unsigned long debounceDelay;
  void (*callback)();  // Callback function khi button được nhấn
  
public:
  ButtonHandler(uint8_t buttonPin, unsigned long debounce = 50);
  void begin();
  void update();
  void setCallback(void (*func)());
  bool isPressed();
};

#endif // BUTTON_HANDLER_H
