/*
 * OTA Manager Module
 * Hỗ trợ Over-The-Air firmware update qua WiFi
 */

#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <ArduinoOTA.h>

class OTAManager {
private:
  String hostname;
  String password;
  bool enabled;
  void (*onStartCallback)();
  void (*onProgressCallback)(unsigned int progress, unsigned int total);
  void (*onEndCallback)();
  
public:
  OTAManager(const char* deviceName, const char* otaPassword = "");
  void begin();
  void handle();
  void setEnabled(bool enable);
  bool isEnabled();
  
  // Callbacks cho UI feedback
  void setOnStart(void (*callback)());
  void setOnProgress(void (*callback)(unsigned int progress, unsigned int total));
  void setOnEnd(void (*callback)());
};

#endif // OTA_MANAGER_H
