/*
 * Button Handler Implementation
 */

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
      mediumPressTriggered = false;
      longPressTriggered = false;
      lastPressTime = now;
    }
  }
  
  // Button is being held down - check for medium press (3s) and long press (7s)
  if (reading == LOW) {
    unsigned long holdDuration = now - pressStartTime;
    
    // Check for long press (7s) - Reset config
    if (!longPressTriggered && holdDuration >= longPressThreshold) {
      longPressTriggered = true;
      Serial.println(F("\n🔘 LONG PRESS detected! (7s) - Reset Config"));
      if (longPressCallback != nullptr) {
        longPressCallback();
      }
    }
    // Check for medium press (3s) - Toggle display
    else if (!mediumPressTriggered && holdDuration >= mediumPressThreshold) {
      mediumPressTriggered = true;
      Serial.println(F("\n🔘 MEDIUM PRESS detected! (3s) - Toggle Display"));
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
        Serial.print(F("Window expired! Reset count. Old: "));
        Serial.print(clickCount);
        clickCount = 1;
        firstClickTime = now;
        Serial.print(F(" -> New: "));
        Serial.println(clickCount);
      }
      
      Serial.print(F("🔘 Click "));
      Serial.print(clickCount);
      Serial.print(F("/"));
      Serial.print(multiClickThreshold);
      Serial.print(F(" | Window: "));
      Serial.print(now - firstClickTime);
      Serial.print(F("ms/"));
      Serial.print(multiClickWindow);
      Serial.println(F("ms"));
      
      // Trigger multi-click callback on 3rd click
      if (clickCount >= multiClickThreshold) {
        Serial.println(F("\n✓✓✓ 3x CLICK DETECTED! Triggering callback..."));
        if (multiClickCallback != nullptr) {
          multiClickCallback();
          Serial.println(F("✓ Callback executed"));
        } else {
          Serial.println(F("⚠️ WARNING: multiClickCallback is NULL!"));
        }
        clickCount = 0;  // Reset
        Serial.println(F("✓ Click count reset to 0\n"));
      }
    }
  }
  
  // Reset click count if window expired
  if (clickCount > 0 && now - firstClickTime > multiClickWindow) {
    Serial.println(F("Multi-click timeout - reset"));
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
