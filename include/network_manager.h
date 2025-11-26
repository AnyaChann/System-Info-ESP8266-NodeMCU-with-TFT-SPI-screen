/*
 * Network Manager Module
 * Quản lý WiFi connection và HTTP requests
 */

#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include "system_data.h"

class NetworkManager {
private:
  const char* ssid;
  const char* password;
  String serverUrl;
  WiFiClient wifiClient;
  unsigned long lastUpdate;
  unsigned long updateInterval;
  
public:
  NetworkManager(const char* wifiSsid, const char* wifiPass, String serverURL, unsigned long interval = 3000);
  bool connectWiFi(int maxAttempts = 20);
  bool isConnected();
  void reconnect();
  bool fetchSystemData(SystemData& data);
  bool shouldUpdate();
  void resetUpdateTimer();
  String getLocalIP();
  
  // Settings management
  void setUpdateInterval(unsigned long interval) { updateInterval = interval; }
  unsigned long getUpdateInterval() const { return updateInterval; }
};

#endif // NETWORK_MANAGER_H
