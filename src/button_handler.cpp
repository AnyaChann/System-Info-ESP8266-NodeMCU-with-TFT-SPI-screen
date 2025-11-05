/*
 * Button Handler Implementation
 */

#include "button_handler.h"

ButtonHandler::ButtonHandler(uint8_t buttonPin, unsigned long debounce)
  : pin(buttonPin), debounceDelay(debounce), lastState(HIGH), 
    lastPressTime(0), callback(nullptr) {}

void ButtonHandler::begin() {
  pinMode(pin, INPUT_PULLUP);
  delay(200);  // Đợi pin ổn định
  
  // Check button nhiều lần để đảm bảo
  for (int i = 0; i < 5; i++) {
    int state = digitalRead(pin);
    Serial.print("Button check #");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(state == HIGH ? "HIGH" : "LOW");
    delay(50);
  }
  
  lastState = digitalRead(pin);
  Serial.print("==> Final button state: ");
  Serial.println(lastState == HIGH ? "HIGH (OK - not pressed)" : "LOW (ERROR - check wiring!)");
}

void ButtonHandler::update() {
  int reading = digitalRead(pin);
  
  // Debug: In state mỗi khi có thay đổi
  if (reading != lastState) {
    Serial.print("Button state changed: ");
    Serial.print(lastState == HIGH ? "HIGH" : "LOW");
    Serial.print(" -> ");
    Serial.println(reading == HIGH ? "HIGH" : "LOW");
  }
  
  // Detect falling edge (HIGH → LOW = button pressed)
  if (reading == LOW && lastState == HIGH) {
    unsigned long now = millis();
    if (now - lastPressTime > debounceDelay) {
      lastPressTime = now;
      Serial.println("=== BUTTON PRESSED ===");
      if (callback != nullptr) {
        callback();
      }
    } else {
      Serial.println("Button press ignored (debounce)");
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
