/*
 * ESP8266 System Monitor với TFT Display 1.8" SPI
 * Refactored Version - Modular Architecture
 * 
 * Modules:
 * - DisplayManager: Quản lý TFT display
 * - NetworkManager: WiFi & HTTP communication
 * - ButtonHandler: Button input với debounce
 * - SystemData: Data structures
 */

#include <Arduino.h>

// Load config
#if __has_include("config.h")
  #include "config.h"
#else
  #error "config.h not found! Copy from config.h.example"
#endif

// Include các module
#include "system_data.h"
#include "display_manager.h"
#include "network_manager.h"
#include "button_handler.h"

// Khởi tạo các manager
DisplayManager display(TFT_CS, TFT_DC, TFT_RST, TFT_LED, SCREEN_ROTATION);
NetworkManager network(WIFI_SSID, WIFI_PASSWORD, 
                       String("http://") + SERVER_IP + ":" + SERVER_PORT + "/system-info");
ButtonHandler button(BUTTON_PIN);
SystemData sysData;

// Callback cho button
void onButtonPressed() {
  display.toggle();
}

void setup() {
  Serial.begin(115200);
  delay(50);
  
  // Init display
  display.begin();
  display.showSplashScreen();
  display.showWiFiConnecting();
  
  // Init button
  button.begin();
  button.setCallback(onButtonPressed);
  
  // Connect WiFi
  bool wifiOk = network.connectWiFi(20);
  display.showWiFiStatus(wifiOk, network.getLocalIP());
  
  display.clear();
}

void loop() {
  static unsigned long buttonEnableTime = 5000;
  unsigned long currentMillis = millis();
  
  // Check button (skip first 5 seconds to avoid boot glitches)
  if (currentMillis > buttonEnableTime) {
    button.update();
  }
  
  // Check WiFi
  if (!network.isConnected()) {
    network.reconnect();
    return;
  }
  
  // Update system data
  if (display.isOn() && network.shouldUpdate()) {
    if (network.fetchSystemData(sysData)) {
      display.displaySystemInfo(sysData);
    }
  }
}

