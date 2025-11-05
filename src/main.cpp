#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

// Load config từ file config.h
// Copy từ config.h.example và chỉnh sửa
#if __has_include("config.h")
  #include "config.h"
  const char* ssid = WIFI_SSID;
  const char* password = WIFI_PASSWORD;
  // Build URL từ IP và PORT
  String serverUrl = String("http://") + SERVER_IP + ":" + SERVER_PORT + "/system-info";
#else
  #warning "config.h not found! Copy from config.h.example and edit it"
  const char* ssid = "YOUR_WIFI_SSID";
  const char* password = "YOUR_WIFI_PASSWORD";
  String serverUrl = "http://127.0.0.1:8080/system-info";
#endif

// Thời gian cập nhật (milliseconds)
unsigned long previousMillis = 0;
const long interval = 2000; // Cập nhật mỗi 2 giây

WiFiClient wifiClient;

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n\n=== ESP8266 System Monitor ===");
  
  // Kết nối WiFi
  WiFi.begin(ssid, password);
  Serial.print("Đang kết nối WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("Kết nối WiFi thành công!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Kiểm tra kết nối WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Mất kết nối WiFi! Đang kết nối lại...");
    WiFi.reconnect();
    delay(5000);
    return;
  }
  
  // Cập nhật thông tin định kỳ
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    getSystemInfo();
  }
}

void getSystemInfo() {
  HTTPClient http;
  
  Serial.println("\n========== SYSTEM INFO ==========");
  
  http.begin(wifiClient, serverUrl.c_str());
  http.setTimeout(5000);
  
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      
      // Parse JSON (tăng buffer lên 2048 để chứa đủ dữ liệu)
      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, payload);
      
      if (error) {
        Serial.print("JSON parse error: ");
        Serial.println(error.c_str());
        http.end();
        return;
      }
      
      // CPU
      if (doc.containsKey("cpu")) {
        JsonObject cpu = doc["cpu"];
        String cpuName = cpu["name"].as<String>();
        if (cpuName.length() > 0) {
          Serial.println("\n[CPU]");
          Serial.println("  " + cpuName);
          Serial.printf("  Temp: %.1f°C | Load: %.1f%% | Power: %.1fW\n", 
            cpu["temp"].as<float>(), 
            cpu["load"].as<float>(),
            cpu["power"].as<float>());
        }
      }
      
      // RAM
      if (doc.containsKey("ram")) {
        JsonObject ram = doc["ram"];
        Serial.println("\n[RAM]");
        Serial.printf("  %.1f / %.1f GB (%.1f%%)\n", 
          ram["used"].as<float>(), 
          ram["total"].as<float>(),
          ram["percent"].as<float>());
      }
      
      // GPU rời (Discrete GPU)
      if (doc.containsKey("gpu_discrete")) {
        JsonObject gpu = doc["gpu_discrete"];
        String gpuName = gpu["name"].as<String>();
        if (gpuName.length() > 0) {
          Serial.println("\n[GPU - Discrete]");
          Serial.println("  " + gpuName);
          float temp = gpu["temp"].as<float>();
          float power = gpu["power"].as<float>();
          int memUsed = gpu["mem_used"].as<int>();
          int memTotal = gpu["mem_total"].as<int>();
          
          Serial.printf("  Temp: %.1f°C | Load: %.1f%%", 
            temp, 
            gpu["load"].as<float>());
          
          if (power > 0) {
            Serial.printf(" | Power: %.1fW", power);
          }
          Serial.println();
          
          if (memTotal > 0) {
            Serial.printf("  VRAM: %d / %d MB\n", memUsed, memTotal);
          }
        }
      }
      
      // iGPU (Integrated GPU)
      if (doc.containsKey("gpu_integrated")) {
        JsonObject gpu = doc["gpu_integrated"];
        String gpuName = gpu["name"].as<String>();
        if (gpuName.length() > 0) {
          Serial.println("\n[iGPU - Integrated]");
          Serial.println("  " + gpuName);
          float temp = gpu["temp"].as<float>();
          if (temp > 0) {
            Serial.printf("  Temp: %.1f°C | Load: %.1f%%\n", 
              temp, 
              gpu["load"].as<float>());
          } else {
            Serial.printf("  Temp: N/A | Load: %.1f%%\n", 
              gpu["load"].as<float>());
          }
        }
      }
      
      // Disk/SSD
      if (doc.containsKey("disk")) {
        JsonArray disks = doc["disk"].as<JsonArray>();
        if (disks.size() > 0) {
          Serial.println("\n[STORAGE]");
          for (JsonObject disk : disks) {
            String diskName = disk["name"].as<String>();
            Serial.println("  " + diskName);
            Serial.printf("  Temp: %.1f°C | Used: %.1f%%\n", 
              disk["temp"].as<float>(), 
              disk["load"].as<float>());
          }
        }
      }
      
      // Network
      if (doc.containsKey("network")) {
        JsonObject net = doc["network"];
        String netName = net["name"].as<String>();
        if (netName.length() > 0) {
          Serial.println("\n[NETWORK - " + netName + "]");
          Serial.printf("  ↓ %.2f KB/s | ↑ %.2f KB/s\n", 
            net["download"].as<float>(), 
            net["upload"].as<float>());
        }
      }
      
      Serial.println("\n=================================");
      
    } else {
      Serial.printf("HTTP Error: %d\n", httpCode);
    }
  } else {
    Serial.print("Lỗi kết nối: ");
    Serial.println(http.errorToString(httpCode).c_str());
  }
  
  http.end();
}