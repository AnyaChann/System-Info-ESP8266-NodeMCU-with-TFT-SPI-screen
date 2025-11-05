/*
 * Config Manager - WiFi và Server Configuration Portal
 * Cho phép config qua Web Portal khi ESP8266 ở AP mode
 * Sử dụng WiFiManager để scan và chọn WiFi
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiManager.h>
#include <EEPROM.h>

// EEPROM Layout
#define EEPROM_SIZE 512
#define EEPROM_MAGIC 0x4553  // "ES" magic number
#define EEPROM_VERSION 1

// Config structure
struct ConfigData {
  uint16_t magic;           // Magic number để verify
  uint8_t version;          // Config version
  
  // Server config (config trước)
  char serverIP[16];        // "192.168.2.60"
  uint16_t serverPort;      // 8080
  
  // WiFi config (config sau)
  char wifiSSID[32];        // WiFi name
  char wifiPassword[64];    // WiFi password
  
  uint8_t checksum;         // Simple checksum
};

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
  
  // Setters (for manual config)
  void setServerIP(const char* ip);
  void setServerPort(uint16_t port);
  void setWiFiCredentials(const char* ssid, const char* pass);

private:
  ConfigData config;
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
  
  // Optional display feedback
  class DisplayManager* displayManager;
  
  // Internal helpers
  uint8_t calculateChecksum();
  bool verifyChecksum();
  void clearConfig();
  
  // WiFiManager callbacks
  void configModeCallback(WiFiManager *myWiFiManager);
  void saveConfigCallback();
  
  // Web handlers for server config
  void handleRoot();
  void handleServerConfig();
  void handleTestServer();
  void handleStatus();
  void handleReset();
  
  // HTML generators
  String generateServerConfigHTML();
  String generateTestingHTML();
  String generateSuccessHTML();
  String generateErrorHTML(const char* error);
};

#endif
