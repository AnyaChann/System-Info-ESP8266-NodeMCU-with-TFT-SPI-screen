/*
 * Button Handler Implementation
 */

#include "config.h"
#include "button_handler.h"

ButtonHandler::ButtonHandler(uint8_t buttonPin, unsigned long debounce, unsigned long mediumPress, unsigned long longPress)
  : pin(buttonPin), lastState(HIGH), lastPressTime(0), pressStartTime(0),
    debounceDelay(debounce), mediumPressThreshold(mediumPress), longPressThreshold(longPress), 
    mediumPressTriggered(false), longPressTriggered(false), 
    callback(nullptr), mediumPressCallback(nullptr), longPressCallback(nullptr),
    multiClickCallback(nullptr), clickCount(0), firstClickTime(0), 
    multiClickWindow(2000), multiClickThreshold(3) {}

void ButtonHandler::begin() {
  pinMode(pin, INPUT_PULLUP);
  delay(100);  // Đợi pin ổn định
  lastState = digitalRead(pin);
  
  #ifdef DEBUG_BUTTON
  DEBUG_PRINTF("[BTN] Initialized on pin %d, state: %s\n", pin, lastState == HIGH ? "HIGH" : "LOW");
  #endif
}

void ButtonHandler::update() {
  int reading = digitalRead(pin);
  unsigned long now = millis();
  
  // Detect falling edge (HIGH → LOW = button pressed)
  if (reading == LOW && lastState == HIGH) {
    if (now - lastPressTime > debounceDelay) {
      pressStartTime = now;
      mediumPressTriggered = false;
      longPressTriggered = false;
      lastPressTime = now;
    }
  }
  
  // Button is being held down - check for medium (3s) and long press (7s)
  if (reading == LOW) {
    unsigned long holdDuration = now - pressStartTime;
    
    // Check for long press (7s) - Reset config (trigger immediately, override medium press)
    if (!longPressTriggered && holdDuration >= longPressThreshold) {
      longPressTriggered = true;
      mediumPressTriggered = true;  // Prevent medium press from triggering
      DEBUG_PRINTLN(F("[BTN] Long press (7s+) - Reset config"));
      if (longPressCallback != nullptr) {
        longPressCallback();
      }
    }
    // Check for medium press (3s) - Toggle display (trigger immediately if not going for long)
    else if (!mediumPressTriggered && holdDuration >= mediumPressThreshold) {
      mediumPressTriggered = true;
      DEBUG_PRINTLN(F("[BTN] Medium press (3s) - Toggle display"));
      if (mediumPressCallback != nullptr) {
        mediumPressCallback();
      }
    }
  }
  
  // Detect rising edge (LOW → HIGH = button released)
  if (reading == HIGH && lastState == LOW) {
    unsigned long pressDuration = now - pressStartTime;
    
    // Only count clicks if no medium/long press was triggered
    if (!mediumPressTriggered && !longPressTriggered && pressDuration < mediumPressThreshold) {
      // Multi-click detection
      if (clickCount == 0) {
        firstClickTime = now;
      }
      
      clickCount++;
      
      // Check if multi-click window expired
      if (now - firstClickTime > multiClickWindow) {
        #ifdef DEBUG_BUTTON
        DEBUG_PRINTF("[BTN] Window expired - Reset count %d -> 1\n", clickCount);
        #endif
        clickCount = 1;
        firstClickTime = now;
      }
      
      #ifdef DEBUG_BUTTON
      DEBUG_PRINTF("[BTN] Click %d/%d (window: %lums)\n", clickCount, multiClickThreshold, now - firstClickTime);
      #endif
      
      // Trigger multi-click callback on 3rd click
      if (clickCount >= multiClickThreshold) {
        DEBUG_PRINTLN(F("[BTN] 3x click detected - OTA mode"));
        if (multiClickCallback != nullptr) {
          multiClickCallback();
        }
        clickCount = 0;  // Reset
      }
    }
  }
  
  // Reset click count if window expired
  if (clickCount > 0 && now - firstClickTime > multiClickWindow) {
    DEBUG_PRINTLN(F("[BTN] Multi-click timeout - reset"));
    clickCount = 0;
  }
  
  lastState = reading;
}

void ButtonHandler::setCallback(void (*func)()) {
  callback = func;
}

void ButtonHandler::setMediumPressCallback(void (*func)()) {
  mediumPressCallback = func;
}

void ButtonHandler::setLongPressCallback(void (*func)()) {
  longPressCallback = func;
}

void ButtonHandler::setMultiClickCallback(void (*func)()) {
  multiClickCallback = func;
}

bool ButtonHandler::isPressed() {
  return digitalRead(pin) == LOW;
}
