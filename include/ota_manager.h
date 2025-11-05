/*
 * OTA Manager Module
 * Hỗ trợ Over-The-Air firmware update qua WiFi
 */

#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <ArduinoOTA.h>

class DisplayManager;  // Forward declaration

class OTAManager {
private:
  String hostname;
  String password;
  bool enabled;
  DisplayManager* display;
  
public:
  OTAManager(const char* deviceName, const char* otaPassword = "");
  void begin();
  void handle();
  void setEnabled(bool enable);
  bool isEnabled();
  void setDisplayManager(DisplayManager* dm);
};

#endif // OTA_MANAGER_H
