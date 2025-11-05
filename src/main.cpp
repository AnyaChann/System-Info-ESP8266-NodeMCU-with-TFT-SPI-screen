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
ConfigManager configMgr("ESP8266-Config", "12345678");  // AP name & password
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
  Serial.println(F("\n⚙️ Entering Config Mode..."));
  
  // Show message on display
  display.clear();
  display.drawText(10, 50, "RESET TO", ST77XX_YELLOW, 2);
  display.drawText(10, 80, "CONFIG MODE", ST77XX_YELLOW, 2);
  delay(2000);
  
  // Reset config and reboot
  configMgr.resetConfig();
  ESP.restart();
}

void onOTAStart() {
  display.clear();
  display.showSplashScreen();
  // Hiển thị "Updating..." trên màn hình
}

void onOTAProgress(unsigned int progress, unsigned int total) {
  // Có thể hiển thị progress bar trên TFT
  unsigned int percent = (progress * 100) / total;
  // TODO: Update progress on display
}

void onOTAEnd() {
  // Hiển thị "Update Complete!" trước khi reboot
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
    // Hiển thị thông tin config portal trên TFT
    display.clear();
    display.drawText(5, 30, "CONFIG MODE", ST77XX_YELLOW, 1);
    display.drawText(5, 50, "Connect WiFi:", ST77XX_WHITE, 1);
    display.drawText(5, 65, "ESP8266-Config", ST77XX_CYAN, 1);
    display.drawText(5, 80, "Pass: 12345678", ST77XX_CYAN, 1);
    display.drawText(5, 100, "Open browser:", ST77XX_WHITE, 1);
    display.drawText(5, 115, "192.168.4.1", ST77XX_GREEN, 1);
    
    // Chờ config từ web portal
    while (configMgr.isConfigMode()) {
      configMgr.handleClient();
      delay(10);
    }
    // Sau khi config xong, ESP sẽ tự reboot
    return;
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
  
  // Init OTA
  #if OTA_ENABLED
  ota.begin();
  ota.setOnStart(onOTAStart);
  ota.setOnProgress(onOTAProgress);
  ota.setOnEnd(onOTAEnd);
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
  
  // Check WiFi connection status (lightweight check)
  if (!network->isConnected()) {
    // Don't call shouldFallbackToConfig() in every loop - it's expensive
    // Only check fallback every 30 seconds
    static unsigned long lastFallbackCheck = 0;
    if (currentMillis - lastFallbackCheck > 30000) {
      lastFallbackCheck = currentMillis;
      
      if (configMgr.shouldFallbackToConfig()) {
        Serial.println(F("\n⚠️ Fallback to Config Mode!"));
        display.clear();
        display.drawText(5, 20, "WiFi ERROR!", ST77XX_RED, 1);
        display.drawText(5, 40, "Failed to", ST77XX_YELLOW, 1);
        display.drawText(5, 55, "connect after", ST77XX_YELLOW, 1);
        display.drawText(5, 70, "5 attempts", ST77XX_YELLOW, 1);
        display.drawText(5, 90, "Rebooting to", ST77XX_WHITE, 1);
        display.drawText(5, 105, "config mode...", ST77XX_WHITE, 1);
        delay(3000);
        
        // Reset config to force portal
        configMgr.resetConfig();
        ESP.restart();
        return;
      }
    }
    
    // Quick reconnect attempt (non-blocking)
    network->reconnect();
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

