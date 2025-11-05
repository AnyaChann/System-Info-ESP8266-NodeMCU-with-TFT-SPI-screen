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
#include "ota_manager.h"
#include "config_manager.h"

// Khởi tạo các manager
ConfigManager configMgr("ESP8266-Config", "82668266");  // AP name & password
DisplayManager display(TFT_CS, TFT_DC, TFT_RST, TFT_LED, SCREEN_ROTATION);
NetworkManager* network = nullptr;  // Khởi tạo sau khi có config
ButtonHandler button(BUTTON_PIN);
OTAManager ota(OTA_HOSTNAME, OTA_PASSWORD);
SystemData sysData;

// Callbacks
void onButtonPressed() {
  display.toggle();
}

void onButtonLongPress() {
  Serial.println(F("\n⚙️ Long press detected - Resetting to Config Mode..."));
  display.clear();
  display.drawText(10, 50, "RESET TO", ST77XX_YELLOW, 2);
  display.drawText(10, 80, "CONFIG MODE", ST77XX_YELLOW, 2);
  delay(2000);
  configMgr.resetConfig();
  ESP.restart();
}

void setup() {
  Serial.begin(115200);
  delay(50);
  
  // Init display
  display.begin();
  display.showSplashScreen();
  
  // Init config manager
  configMgr.setDisplayManager(&display);  // Pass display for reconnect feedback
  configMgr.begin();
  
  // Check if in config mode (no valid config or user reset)
  if (configMgr.isConfigMode()) {
    while (configMgr.isConfigMode()) {
      configMgr.handleClient();
      delay(10);
    }
    return;  // Config complete, ESP will reboot
  }
  
  // Có config rồi, khởi tạo network với config đã lưu
  network = new NetworkManager(
    configMgr.getWiFiSSID().c_str(),
    configMgr.getWiFiPassword().c_str(),
    configMgr.getServerURL()
  );
  
  display.showWiFiConnecting();
  
  // Init button
  button.begin();
  button.setCallback(onButtonPressed);           // Short press: toggle display
  button.setLongPressCallback(onButtonLongPress); // Long press (5s): config mode
  
  // Connect WiFi với timeout hợp lý
  Serial.println(F("Connecting to WiFi..."));
  bool wifiOk = network->connectWiFi(20); // 20 attempts * 0.5s = 10s max
  
  if (wifiOk) {
    Serial.print(F("✓ WiFi connected! IP: "));
    Serial.println(network->getLocalIP());
    // Keep "Connecting..." screen until first data fetch
  } else {
    display.showWiFiStatus(false, "");
    Serial.println(F("✗ WiFi connection failed!"));
    Serial.println(F("Will retry in loop..."));
  }
  
  // Init OTA with display feedback
  #if OTA_ENABLED
  ota.setDisplayManager(&display);
  ota.begin();
  Serial.println(F("✓ OTA enabled - Ready for wireless updates"));
  #endif
}

void loop() {
  // Nếu đang ở config mode, chỉ handle web requests
  if (configMgr.isConfigMode()) {
    configMgr.handleClient();
    return;
  }
  
  // Network chưa được khởi tạo (lỗi config)
  if (network == nullptr) {
    return;
  }
  
  static unsigned long buttonEnableTime = 5000;
  unsigned long currentMillis = millis();
  
  // Check WiFi - shouldFallbackToConfig() handles display & reset
  if (!network->isConnected()) {
    if (configMgr.shouldFallbackToConfig()) {
      configMgr.resetConfig();
      ESP.restart();
    }
    return;
  }
  
  // Handle OTA updates
  #if OTA_ENABLED
  ota.handle();
  #endif
  
  // Check button (skip first 5 seconds to avoid boot glitches)
  if (currentMillis > buttonEnableTime) {
    button.update();
  }
  
  // Update system data (only if WiFi connected and display on)
  if (display.isOn() && network->shouldUpdate()) {
    if (network->fetchSystemData(sysData)) {
      display.displaySystemInfo(sysData);
      configMgr.reportServerSuccess();  // Reset server fail counter
    } else {
      // Fetch failed - report to config manager
      static unsigned long lastErrorDisplay = 0;
      if (currentMillis - lastErrorDisplay > 5000) {
        lastErrorDisplay = currentMillis;
        Serial.println(F("⚠️ Failed to fetch system data"));
      }
      configMgr.reportServerFailure();  // Track server failures
    }
  }
}

