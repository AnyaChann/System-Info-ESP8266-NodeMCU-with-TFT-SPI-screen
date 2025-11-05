/*
 * Network Manager Implementation
 */

#include "network_manager.h"
#include <ArduinoJson.h>

NetworkManager::NetworkManager(const char* wifiSsid, const char* wifiPass, String serverURL, unsigned long interval)
  : ssid(wifiSsid), password(wifiPass), serverUrl(serverURL), 
    updateInterval(interval), lastUpdate(0) {}

bool NetworkManager::connectWiFi(int maxAttempts) {
  WiFi.begin(ssid, password);
  int attempts = 0;
  
  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }
  
  Serial.println("\nWiFi failed!");
  return false;
}

bool NetworkManager::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void NetworkManager::reconnect() {
  Serial.println("WiFi lost! Reconnecting...");
  WiFi.reconnect();
  delay(5000);
}

bool NetworkManager::fetchSystemData(SystemData& data) {
  if (!isConnected()) {
    return false;
  }
  
  HTTPClient http;
  http.begin(wifiClient, serverUrl.c_str());
  http.setTimeout(5000);
  
  int httpCode = http.GET();
  bool success = false;
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      // Parse CPU
      data.cpuName = doc["cpu"]["name"].as<String>();
      data.cpuTemp = doc["cpu"]["temp"].as<float>();
      data.cpuLoad = doc["cpu"]["load"].as<float>();
      data.cpuPower = doc["cpu"]["power"].as<float>();
      
      // Parse RAM
      data.ramUsed = doc["ram"]["used"].as<float>();
      data.ramTotal = doc["ram"]["total"].as<float>();
      data.ramPercent = doc["ram"]["percent"].as<float>();
      
      // Parse GPU
      data.gpuName = doc["gpu_discrete"]["name"].as<String>();
      data.gpuTemp = doc["gpu_discrete"]["temp"].as<float>();
      data.gpuLoad = doc["gpu_discrete"]["load"].as<float>();
      data.gpuPower = doc["gpu_discrete"]["power"].as<float>();
      data.gpuMemUsed = doc["gpu_discrete"]["mem_used"].as<int>();
      data.gpuMemTotal = doc["gpu_discrete"]["mem_total"].as<int>();
      
      // Parse Disks
      JsonArray disks = doc["disk"].as<JsonArray>();
      if (disks.size() > 0) {
        data.disk1Name = disks[0]["name"].as<String>();
        data.disk1Temp = disks[0]["temp"].as<float>();
        data.disk1Load = disks[0]["load"].as<float>();
      }
      if (disks.size() > 1) {
        data.disk2Name = disks[1]["name"].as<String>();
        data.disk2Temp = disks[1]["temp"].as<float>();
        data.disk2Load = disks[1]["load"].as<float>();
      }
      
      // Parse Network
      data.netName = doc["network"]["name"].as<String>();
      data.netDown = doc["network"]["download"].as<float>();
      data.netUp = doc["network"]["upload"].as<float>();
      
      data.hasData = true;
      success = true;
      Serial.println("Data updated!");
    } else {
      Serial.println("JSON parse error!");
      data.hasData = false;
    }
  } else {
    Serial.printf("HTTP error: %d\n", httpCode);
    data.hasData = false;
  }
  
  http.end();
  return success;
}

bool NetworkManager::shouldUpdate() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdate >= updateInterval) {
    lastUpdate = currentMillis;
    return true;
  }
  return false;
}

void NetworkManager::resetUpdateTimer() {
  lastUpdate = millis();
}

String NetworkManager::getLocalIP() {
  return WiFi.localIP().toString();
}
