/*
 * OTA Web Manager Module
 * Quản lý web-based firmware update với HTTP server
 */

#ifndef OTA_WEB_MANAGER_H
#define OTA_WEB_MANAGER_H

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include "display_manager.h"

class OTAWebManager {
private:
  DisplayManager* display;
  ESP8266WebServer* webServer;
  ESP8266HTTPUpdateServer* httpUpdater;
  bool isActive;
  String currentIP;
  
  void showActiveScreen();
  void showClosedScreen();
  String generateRootHTML();
  String generateUpdateHTML();
  void handleUpdatePage();
  void handleUpload();
  void handleUploadCallback();
  
public:
  OTAWebManager();
  ~OTAWebManager();
  
  void setDisplayManager(DisplayManager* dm);
  void start(const String& ipAddress);
  void stop();
  void handle();
  bool active() const;
};

#endif // OTA_WEB_MANAGER_H
