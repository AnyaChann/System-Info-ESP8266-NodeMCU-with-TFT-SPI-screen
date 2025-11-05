/*
 * Button Handler Implementation
 */

#include "button_handler.h"

ButtonHandler::ButtonHandler(uint8_t buttonPin, unsigned long debounce, unsigned long longPress)
  : pin(buttonPin), lastState(HIGH), lastPressTime(0), pressStartTime(0),
    debounceDelay(debounce), longPressThreshold(longPress), 
    longPressTriggered(false), callback(nullptr), longPressCallback(nullptr) {}

void ButtonHandler::begin() {
  pinMode(pin, INPUT_PULLUP);
  delay(100);  // Đợi pin ổn định
  lastState = digitalRead(pin);
  
  #ifdef DEBUG_BUTTON
  Serial.print(F("Button initialized on pin "));
  Serial.print(pin);
  Serial.print(F(", state: "));
  Serial.println(lastState == HIGH ? F("HIGH") : F("LOW"));
  #endif
}

void ButtonHandler::update() {
  int reading = digitalRead(pin);
  unsigned long now = millis();
  
  // Detect falling edge (HIGH → LOW = button pressed)
  if (reading == LOW && lastState == HIGH) {
    if (now - lastPressTime > debounceDelay) {
      pressStartTime = now;
      longPressTriggered = false;
      lastPressTime = now;
    }
  }
  
  // Button is being held down - check for long press
  if (reading == LOW && !longPressTriggered) {
    if (now - pressStartTime >= longPressThreshold) {
      longPressTriggered = true;
      Serial.println(F("\n🔘 LONG PRESS detected! (5s)"));
      if (longPressCallback != nullptr) {
        longPressCallback();
      }
    }
  }
  
  // Detect rising edge (LOW → HIGH = button released)
  if (reading == HIGH && lastState == LOW) {
    unsigned long pressDuration = now - pressStartTime;
    
    // Only trigger short press if long press wasn't triggered
    if (!longPressTriggered && pressDuration < longPressThreshold) {
      if (callback != nullptr) {
        callback();
      }
    }
  }
  
  lastState = reading;
}

void ButtonHandler::setCallback(void (*func)()) {
  callback = func;
}

void ButtonHandler::setLongPressCallback(void (*func)()) {
  longPressCallback = func;
}

bool ButtonHandler::isPressed() {
  return digitalRead(pin) == LOW;
}
