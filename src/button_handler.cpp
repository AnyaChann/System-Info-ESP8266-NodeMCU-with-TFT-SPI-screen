/*
 * Button Handler Implementation
 */

#include "button_handler.h"

ButtonHandler::ButtonHandler(uint8_t buttonPin, unsigned long debounce)
  : pin(buttonPin), lastState(HIGH), lastPressTime(0), 
    debounceDelay(debounce), callback(nullptr) {}

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
  
  // Detect falling edge (HIGH → LOW = button pressed)
  if (reading == LOW && lastState == HIGH) {
    unsigned long now = millis();
    if (now - lastPressTime > debounceDelay) {
      lastPressTime = now;
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

bool ButtonHandler::isPressed() {
  return digitalRead(pin) == LOW;
}
