/*
 * OTA Manager Implementation
 */

#include "ota_manager.h"

OTAManager::OTAManager(const char* deviceName, const char* otaPassword)
  : hostname(deviceName), password(otaPassword), enabled(true),
    onStartCallback(nullptr), onProgressCallback(nullptr), onEndCallback(nullptr) {}

void OTAManager::begin() {
  if (!enabled) return;
  
  // Set hostname
  ArduinoOTA.setHostname(hostname.c_str());
  
  // Set password (nếu có)
  if (password.length() > 0) {
    ArduinoOTA.setPassword(password.c_str());
  }
  
  // Callbacks
  ArduinoOTA.onStart([this]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    
    #ifdef DEBUG_OTA
    Serial.println(F("OTA Update Start"));
    Serial.print(F("Type: "));
    Serial.println(type);
    #endif
    
    if (onStartCallback) {
      onStartCallback();
    }
  });
  
  ArduinoOTA.onEnd([this]() {
    #ifdef DEBUG_OTA
    Serial.println(F("\nOTA Update Complete!"));
    #endif
    
    if (onEndCallback) {
      onEndCallback();
    }
  });
  
  ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
    #ifdef DEBUG_OTA
    static unsigned int lastPercent = 0;
    unsigned int percent = (progress / (total / 100));
    if (percent != lastPercent && percent % 10 == 0) {
      Serial.printf("Progress: %u%%\n", percent);
      lastPercent = percent;
    }
    #endif
    
    if (onProgressCallback) {
      onProgressCallback(progress, total);
    }
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    #ifdef DEBUG_OTA
    Serial.printf("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println(F("Auth Failed"));
    else if (error == OTA_BEGIN_ERROR) Serial.println(F("Begin Failed"));
    else if (error == OTA_CONNECT_ERROR) Serial.println(F("Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Receive Failed"));
    else if (error == OTA_END_ERROR) Serial.println(F("End Failed"));
    #endif
  });
  
  ArduinoOTA.begin();
  
  #ifdef DEBUG_OTA
  Serial.println(F("OTA Ready"));
  Serial.print(F("Hostname: "));
  Serial.println(hostname);
  Serial.print(F("Password: "));
  Serial.println(password.length() > 0 ? "******" : "None");
  #endif
}

void OTAManager::handle() {
  if (enabled) {
    ArduinoOTA.handle();
  }
}

void OTAManager::setEnabled(bool enable) {
  enabled = enable;
}

bool OTAManager::isEnabled() {
  return enabled;
}

void OTAManager::setOnStart(void (*callback)()) {
  onStartCallback = callback;
}

void OTAManager::setOnProgress(void (*callback)(unsigned int progress, unsigned int total)) {
  onProgressCallback = callback;
}

void OTAManager::setOnEnd(void (*callback)()) {
  onEndCallback = callback;
}
