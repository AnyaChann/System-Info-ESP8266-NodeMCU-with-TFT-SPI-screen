/*
 * Config Manager - WiFi và Server Configuration Portal
 * Modular architecture with separated storage and portal
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiManager.h>
#include "config_storage.h"
#include "config_portal.h"

class ConfigManager {
public:
  ConfigManager(const char* apName = "ESP8266-Config", const char* apPass = "12345678");
  
  // Lifecycle
  void begin();
  bool loadConfig();
  bool saveConfig();
  void resetConfig();
  
  // Config data access
  String getServerIP() { return String(config.serverIP); }
  uint16_t getServerPort() { return config.serverPort; }
  String getWiFiSSID() { return String(config.wifiSSID); }
  String getWiFiPassword() { return String(config.wifiPassword); }
  String getServerURL();
  
  bool hasValidConfig();
  bool isConfigMode() { return configMode; }
  
  // Config Portal với WiFiManager
  bool startConfigPortalWithValidation();
  void handleClient();
  
  // Validation
  bool testWiFiConnection(const char* ssid, const char* pass, int timeout = 15000);
  bool testServerConnection(const char* serverIP, uint16_t serverPort, int timeout = 5000);
  
  // Fallback detection
  bool shouldFallbackToConfig();
  
  // Display feedback (optional)
  void setDisplayManager(class DisplayManager* disp) { displayManager = disp; }
  
private:
  // Display helpers
  void showReconnectDisplay();
  void showRetryDisplay();
  
  // Portal helpers
  bool startServerConfigPortal();
  bool startWiFiConfigPortal();
  
  // Setters (for manual config)
  void setServerIP(const char* ip);
  void setServerPort(uint16_t port);
  void setWiFiCredentials(const char* ssid, const char* pass);

private:
  ConfigData config;
  ConfigStorage storage;
  WiFiManager* wifiManager;
  ESP8266WebServer* server;
  bool configMode;
  const char* apSSID;
  const char* apPassword;
  
  // Validation state
  String tempServerIP;
  uint16_t tempServerPort;
  int connectionFailCount;
  unsigned long lastConnectionAttempt;
  bool lastConnectedShown;
  
  // Optional display feedback
  class DisplayManager* displayManager;
  
  // WiFiManager callbacks
  void configModeCallback(WiFiManager *myWiFiManager);
  void saveConfigCallback();
  
  // Web handlers for server config
  void handleRoot();
  void handleServerConfig();
  void handleTestServer();
  void handleStatus();
  void handleReset();
};

#endif
