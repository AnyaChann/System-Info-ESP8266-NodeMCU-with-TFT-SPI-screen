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
  unsigned long mediumPressThreshold;  // 3s - Toggle display
  unsigned long longPressThreshold;    // 7s - Reset config
  bool mediumPressTriggered;
  bool longPressTriggered;
  void (*callback)();              // Short press callback (unused now)
  void (*mediumPressCallback)();   // Medium press (3s) - Toggle display
  void (*longPressCallback)();     // Long press (7s) - Reset config
  void (*multiClickCallback)();    // Multi-click callback (3x in 2s) - OTA mode
  
  // Multi-click detection
  uint8_t clickCount;
  unsigned long firstClickTime;
  unsigned long multiClickWindow;      // Time window for multi-click (2000ms)
  uint8_t multiClickThreshold;         // Number of clicks needed (3)
  
public:
  ButtonHandler(uint8_t buttonPin, unsigned long debounce = 50, unsigned long mediumPress = 2000, unsigned long longPress = 7000);
  void begin();
  void update();
  void setCallback(void (*func)());
  void setShortPressCallback(void (*func)()) { callback = func; }  // Alias for menu navigation
  void setMediumPressCallback(void (*func)());
  void setLongPressCallback(void (*func)());
  void setMultiClickCallback(void (*func)());
  bool isPressed();
};

#endif // BUTTON_HANDLER_H
