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
  unsigned long pressStartTime;
  unsigned long debounceDelay;
  unsigned long longPressThreshold;
  bool longPressTriggered;
  void (*callback)();          // Short press callback
  void (*longPressCallback)(); // Long press callback
  
public:
  ButtonHandler(uint8_t buttonPin, unsigned long debounce = 50, unsigned long longPress = 5000);
  void begin();
  void update();
  void setCallback(void (*func)());
  void setLongPressCallback(void (*func)());
  bool isPressed();
};

#endif // BUTTON_HANDLER_H
