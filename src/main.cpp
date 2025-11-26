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
#include "version.h"
#include "system_data.h"
#include "display_manager.h"
#include "network_manager.h"
#include "button_handler.h"
#include "ota_manager.h"
#include "ota_web_manager.h"
#include "config_manager.h"
#include "settings_manager.h"
#include "menu_manager.h"

// Khởi tạo các manager
ConfigManager configMgr("ESP8266-Config", "82668266");  // AP name & password
SettingsManager settingsMgr;
DisplayManager display(TFT_CS, TFT_DC, TFT_RST, TFT_LED, SCREEN_ROTATION);
NetworkManager* network = nullptr;  // Khởi tạo sau khi có config
ButtonHandler button(BUTTON_PIN);
OTAManager ota(OTA_HOSTNAME, OTA_PASSWORD);
OTAWebManager otaWeb;
MenuManager* menu = nullptr;  // Khởi tạo sau khi có display
SystemData sysData;

// Global flags
bool forceRefreshSystemInfo = false;

// Menu exit callback
void onMenuExit() {
  forceRefreshSystemInfo = true;
  // Reset reconnect fail counter after menu (not a WiFi issue)
  configMgr.reportServerSuccess();
  DEBUG_PRINTLN(F("[MAIN] Menu exit - forcing refresh"));
}

// Callbacks
void onButtonShortPress() {
  // Short press = Navigate menu (if active)
  if (menu && menu->isActive()) {
    menu->next();
  }
}

void onButtonMediumPress() {
  // 2s hold = Select menu item / Enter menu
  if (menu) {
    if (menu->isActive()) {
      // In menu: select current item
      menu->select();
    } else {
      // Not in menu: enter menu
      menu->enter();
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(50);
  
  // Disable WiFi debug output (prevents garbage characters at boot)
  Serial.setDebugOutput(false);
  WiFi.setOutputPower(20.5);  // Max WiFi power
  
  // Print version info
  #ifdef DEBUG_MODE
  Serial.println(F("\n========================================"));
  Serial.print(F("  "));
  Serial.println(F(VERSION_FULL));
  Serial.print(F("  Build: "));
  Serial.print(F(BUILD_DATE));
  Serial.print(F(" "));
  Serial.println(F(BUILD_TIME));
  Serial.println(F("========================================\n"));
  #endif
  
  // Init display
  display.begin();
  display.showSplashScreen();
  
  // Init settings manager (before menu)
  settingsMgr.begin();
  
  // Init button FIRST - có thể dùng bất cứ lúc nào
  button.begin();
  button.setShortPressCallback(onButtonShortPress);    // Short press: menu navigation
  button.setMediumPressCallback(onButtonMediumPress);  // 2s hold: menu select/enter
  
  // Init OTA Web Manager
  otaWeb.setDisplayManager(&display);
  
  // Init config manager
  configMgr.setDisplayManager(&display);
  configMgr.setButtonHandler(&button);  // Pass display for reconnect feedback
  configMgr.begin();
  
  // Check if in config mode (no valid config or user reset)
  if (configMgr.isConfigMode()) {
    while (configMgr.isConfigMode()) {
      configMgr.handleClient();
      button.update();  // Allow button during config mode
      delay(10);
    }
    return;  // Config complete, ESP will reboot
  }
  
  // Có config rồi, khởi tạo network với refresh rate từ settings
  network = new NetworkManager(
    configMgr.getWiFiSSID().c_str(),
    configMgr.getWiFiPassword().c_str(),
    configMgr.getServerURL(),
    settingsMgr.getRefreshInterval()  // Use saved refresh rate
  );
  
  // Init menu manager (after all dependencies ready)
  menu = new MenuManager(&display, &settingsMgr, &configMgr, &otaWeb);
  menu->setExitCallback(onMenuExit);  // Force refresh on menu exit
  
  display.showWiFiConnecting();
  
  // Connect WiFi với timeout hợp lý - allow button during connection
  DEBUG_PRINTLN(F("[WIFI] Connecting..."));
  int attempts = 0;
  const int maxAttempts = 20;
  
  while (!network->isConnected() && attempts < maxAttempts) {
    network->connectWiFi(1);  // Try 1 attempt at a time
    button.update();  // Check button during WiFi connect
    attempts++;
    delay(500);
  }
  
  if (network->isConnected()) {
    DEBUG_PRINT(F("[WIFI] Connected! IP: "));
    DEBUG_PRINTLN(network->getLocalIP());
  } else {
    display.showWiFiStatus(false, "");
    DEBUG_PRINTLN(F("[WIFI] Connection failed - will retry in loop"));
  }
  
  // Init OTA with display feedback
  #if OTA_ENABLED
  ota.setDisplayManager(&display);
  ota.begin();
  DEBUG_PRINTLN(F("[OTA] Ready for wireless updates"));
  #endif
}

void loop() {
  // Button ALWAYS active - can reset anytime
  button.update();
  
  // Update menu (handle timeout)
  if (menu) {
    menu->update();
  }
  
  // Nếu đang ở menu mode - skip system info update
  if (menu && menu->isActive()) {
    return;
  }
  
  // Nếu đang ở OTA mode - handle web server (non-blocking!)
  if (otaWeb.active()) {
    otaWeb.handle();
    return;
  }
  
  // Nếu đang ở config mode, chỉ handle web requests
  if (configMgr.isConfigMode()) {
    configMgr.handleClient();
    return;
  }
  
  // Network chưa được khởi tạo (lỗi config)
  if (network == nullptr) {
    return;
  }
  
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
  
  // Update system data (only if WiFi connected and display on)
  // Force update if menu just exited OR normal refresh interval passed
  if (display.isOn() && (forceRefreshSystemInfo || network->shouldUpdate())) {
    // Update network refresh interval from settings (in case changed via menu)
    network->setUpdateInterval(settingsMgr.getRefreshInterval());
    
    if (network->fetchSystemData(sysData)) {
      display.displaySystemInfo(sysData);
      configMgr.reportServerSuccess();  // Reset server fail counter
      forceRefreshSystemInfo = false;   // Clear force refresh flag
    } else {
      // Fetch failed - report to config manager
      DEBUG_PRINTLN(F("[DATA] Failed to fetch system data"));
      configMgr.reportServerFailure();  // Track server failures
      forceRefreshSystemInfo = false;   // Clear flag even on failure
    }
  }
}

