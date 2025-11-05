/*
 * Config Manager Implementation - Lightweight with modular dependencies
 */

#include "config.h"  // Must include first for TFT type definition
#include "config_manager.h"
#include "config_validator.h"
#include "display_manager.h"

ConfigManager::ConfigManager(const char* apName, const char* apPass)
  : wifiManager(nullptr), server(nullptr), configMode(false), 
    apSSID(apName), apPassword(apPass),
    tempServerPort(8080), connectionFailCount(0), serverFailCount(0),
    lastConnectionAttempt(0), lastServerCheck(0), displayManager(nullptr) {
  storage.clear(config);
}

void ConfigManager::begin() {
  // Try to load saved config using storage module
  if (!storage.load(config)) {
    DEBUG_PRINTLN(F("[CFG] No valid config found, starting config portal"));
    startConfigPortalWithValidation();
  } else {
    DEBUG_PRINTLN(F("[CFG] Config loaded successfully"));
    DEBUG_PRINT(F("[CFG] Server: "));
    DEBUG_PRINTLN(getServerURL());
    DEBUG_PRINT(F("[CFG] WiFi: "));
    DEBUG_PRINTLN(getWiFiSSID());
  }
}

bool ConfigManager::loadConfig() {
  return storage.load(config);
}

bool ConfigManager::saveConfig() {
  return storage.save(config);
}

void ConfigManager::resetConfig() {
  storage.clear(config);
  saveConfig();
  DEBUG_PRINTLN(F("[CFG] Config reset!"));
}

bool ConfigManager::hasValidConfig() {
  return storage.hasValidConfig(config);
}

String ConfigManager::getServerURL() {
  return String("http://") + config.serverIP + ":" + config.serverPort + "/system-info";
}

void ConfigManager::setServerIP(const char* ip) {
  strncpy(config.serverIP, ip, sizeof(config.serverIP) - 1);
  config.serverIP[sizeof(config.serverIP) - 1] = '\0';
  DEBUG_PRINT(F("[CFG] Set Server IP: "));
  DEBUG_PRINTLN(config.serverIP);
}

void ConfigManager::setServerPort(uint16_t port) {
  config.serverPort = port;
  DEBUG_PRINT(F("[CFG] Set Server Port: "));
  DEBUG_PRINTLN(config.serverPort);
}

void ConfigManager::setWiFiCredentials(const char* ssid, const char* pass) {
  strncpy(config.wifiSSID, ssid, sizeof(config.wifiSSID) - 1);
  config.wifiSSID[sizeof(config.wifiSSID) - 1] = '\0';
  
  strncpy(config.wifiPassword, pass, sizeof(config.wifiPassword) - 1);
  config.wifiPassword[sizeof(config.wifiPassword) - 1] = '\0';
  
  DEBUG_PRINT(F("[CFG] Set WiFi SSID: "));
  DEBUG_PRINTLN(config.wifiSSID);
  DEBUG_PRINT(F("[CFG] Set WiFi Pass: "));
  DEBUG_PRINTLN(config.wifiPassword[0] ? "****" : "(empty)");
}

// ============= Validation (delegated to ConfigValidator) =============

bool ConfigManager::testWiFiConnection(const char* ssid, const char* pass, int timeout) {
  return ConfigValidator::testWiFi(ssid, pass, timeout);
}

bool ConfigManager::testServerConnection(const char* serverIP, uint16_t serverPort, int timeout) {
  return ConfigValidator::testServer(serverIP, serverPort, timeout);
}

// ============= Fallback Detection =============

bool ConfigManager::shouldFallbackToConfig() {
  // WiFi connected - reset WiFi counter and return
  if (WiFi.status() == WL_CONNECTED) {
    connectionFailCount = 0;
    // Note: serverFailCount is managed separately by reportServerFailure()
    return false;
  }
  
  // Debounce check
  unsigned long currentTime = millis();
  if (currentTime - lastConnectionAttempt < 10000) return false;
  
  connectionFailCount++;
  lastConnectionAttempt = currentTime;
  DEBUG_PRINT(F("[CFG] WiFi fail: "));
  DEBUG_PRINTLN(connectionFailCount);
  
  // Try reconnect
  if (strlen(config.wifiSSID) > 0) {
    WiFi.disconnect();
    delay(100);
    WiFi.begin(config.wifiSSID, config.wifiPassword);
    
    // Progressive timeout: 10s->30s
    int timeout = min(5000 + (connectionFailCount * 5000), 30000);
    unsigned long startTime = millis();
    int dots = 0;
    
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout) {
      // Update display with dots animation every 500ms
      if (displayManager && (millis() - startTime) % 500 < 50) {
        showReconnectDisplay();
        // Add animated dots
        displayManager->drawText(10, 130, "          ", ST77XX_BLACK, 1); // Clear
        for (int i = 0; i < (dots % 4); i++) {
          displayManager->drawText(10 + (i * 10), 130, ".", ST77XX_YELLOW, 2);
        }
        dots++;
      }
      
      delay(100);
      DEBUG_PRINT(F("."));
    }
    DEBUG_PRINTLN("");
    
    if (WiFi.status() == WL_CONNECTED) {
      DEBUG_PRINT(F("[CFG] Reconnected! IP: "));
      DEBUG_PRINTLN(WiFi.localIP());
      
      // No display - let main loop show system info directly
      connectionFailCount = 0;
      return false;
    }
    
    DEBUG_PRINT(F("[CFG] Failed ("));
    DEBUG_PRINT(timeout / 1000);
    DEBUG_PRINTLN(F("s)"));
    if (displayManager) showRetryDisplay();
  }
  
  // Check if reached max fails
  if (connectionFailCount >= 5) {
    DEBUG_PRINTLN(F("\n[CFG] TOO MANY FAILURES! Entering config mode...\n"));
    return true;
  }
  
  return false;
}

void ConfigManager::showReconnectDisplay() {
  if (!displayManager) return;
  
  displayManager->clear();
  displayManager->drawText(10, 40, "WiFi Lost!", ST77XX_RED, 2);
  displayManager->drawText(10, 70, "Reconnecting", ST77XX_YELLOW, 1);
  
  char attemptText[16];
  snprintf(attemptText, sizeof(attemptText), "Attempt %d/5", connectionFailCount);
  displayManager->drawText(10, 90, attemptText, ST77XX_WHITE, 1);
  displayManager->drawText(10, 110, config.wifiSSID, ST77XX_CYAN, 1);
}

void ConfigManager::showRetryDisplay() {
  if (!displayManager) return;
  
  displayManager->clear();
  displayManager->drawText(5, 50, "Retry...", ST77XX_YELLOW, 2);
  char attemptText[32];
  snprintf(attemptText, sizeof(attemptText), "%d/5", connectionFailCount);
  displayManager->drawText(45, 80, attemptText, ST77XX_WHITE, 2);
}

void ConfigManager::reportServerFailure() {
  unsigned long currentTime = millis();
  
  // Debounce: only count if more than 10s since last check
  if (currentTime - lastServerCheck < 10000) return;
  
  lastServerCheck = currentTime;
  serverFailCount++;
  
  DEBUG_PRINT(F("[CFG] Server fail: "));
  DEBUG_PRINT(serverFailCount);
  DEBUG_PRINTLN(F("/10"));
  
  // Show server reconnecting on display
  if (displayManager) {
    displayManager->clear();
    displayManager->drawText(5, 40, "Server Lost!", ST77XX_RED, 2);
    displayManager->drawText(5, 70, "Reconnecting", ST77XX_YELLOW, 1);
    
    char attemptText[20];
    snprintf(attemptText, sizeof(attemptText), "Attempt %d/10", serverFailCount);
    displayManager->drawText(5, 90, attemptText, ST77XX_WHITE, 1);
    
    displayManager->drawText(5, 110, config.serverIP, ST77XX_CYAN, 1);
  }
  
  // If server fails 10 times, fallback to config
  if (serverFailCount >= 10) {
    DEBUG_PRINTLN(F("\n[CFG] SERVER UNREACHABLE! Entering config mode...\n"));
    if (displayManager) {
      displayManager->clear();
      displayManager->drawText(5, 20, "SERVER ERROR!", ST77XX_RED, 1);
      displayManager->drawText(5, 40, "Cannot reach", ST77XX_YELLOW, 1);
      displayManager->drawText(5, 55, "server after", ST77XX_YELLOW, 1);
      displayManager->drawText(5, 70, "10 attempts", ST77XX_YELLOW, 1);
      displayManager->drawText(5, 90, "Rebooting to", ST77XX_WHITE, 1);
      displayManager->drawText(5, 105, "config mode...", ST77XX_WHITE, 1);
      delay(3000);
    }
    resetConfig();
    ESP.restart();
  }
}

void ConfigManager::reportServerSuccess() {
  serverFailCount = 0;  // Reset on success
}

// ============= Config Portal =============

bool ConfigManager::startConfigPortalWithValidation() {
  configMode = true;
  DEBUG_PRINTLN(F("\n[CFG] === CONFIG PORTAL ==="));
  
  // Step 1: Server config
  if (!startServerConfigPortal()) return false;
  
  // Step 2: WiFi config
  if (!startWiFiConfigPortal()) return false;
  
  // Step 3: Validate server (optional - save anyway if failed)
  DEBUG_PRINTLN(F("\n[CFG] Step 3: Validating..."));
  bool serverValid = testServerConnection(tempServerIP.c_str(), tempServerPort, 5000);
  
  if (!serverValid) {
    DEBUG_PRINTLN(F("[CFG] Warning: Server validation failed!"));
    DEBUG_PRINTLN(F("[CFG] Config will be saved anyway. You can fix server later."));
    
    if (displayManager) {
      displayManager->clear();
      displayManager->drawText(20, 40, "WARNING", ST77XX_YELLOW, 2);
      displayManager->drawText(5, 75, "Server offline", ST77XX_WHITE, 1);
      displayManager->drawText(5, 95, "Saving anyway", ST77XX_CYAN, 1);
      delay(2000);
    }
  } else {
    DEBUG_PRINTLN(F("[CFG] Server validated!"));
  }
  
  // Step 4: Save config (even if server validation failed)
  DEBUG_PRINTLN(F("\n[CFG] Step 4: Saving..."));
  setServerIP(tempServerIP.c_str());
  setServerPort(tempServerPort);
  setWiFiCredentials(WiFi.SSID().c_str(), WiFi.psk().c_str());
  
  if (saveConfig()) {
    DEBUG_PRINTLN(F("[CFG] Config saved!"));
    
    if (displayManager) {
      displayManager->clear();
      displayManager->drawText(30, 50, "SAVED!", ST77XX_GREEN, 2);
      displayManager->drawText(20, 90, "Reboot in", ST77XX_WHITE, 1);
      displayManager->drawText(40, 110, "3 sec", ST77XX_CYAN, 2);
    }
    
    configMode = false;
    delay(3000);
    ESP.restart();
    return true;
  }
  
  DEBUG_PRINTLN(F("[CFG] Save failed!"));
  
  if (displayManager) {
    displayManager->clear();
    displayManager->drawText(10, 60, "ERROR!", ST77XX_RED, 2);
    displayManager->drawText(20, 95, "Save failed", ST77XX_WHITE, 1);
    delay(3000);
  }
  
  return false;
}

bool ConfigManager::startServerConfigPortal() {
  DEBUG_PRINTLN(F("[CFG] Step 1: Server Config"));
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID, apPassword);
  IPAddress apIP = WiFi.softAPIP();
  
  DEBUG_PRINT(F("[CFG] AP: "));
  DEBUG_PRINTLN(apIP);
  
  // Show AP info on display (compact and clear)
  if (displayManager) {
    displayManager->clear();
    displayManager->drawText(15, 15, "IP SETUP", ST77XX_YELLOW, 2);
    
    displayManager->drawText(5, 50, "WiFi:", ST77XX_WHITE, 1);
    displayManager->drawText(5, 65, apSSID, ST77XX_CYAN, 1);
    displayManager->drawText(5, 75, apPassword, ST77XX_CYAN, 1);
    
    displayManager->drawText(5, 90, "Browse for:", ST77XX_WHITE, 1);
    displayManager->drawText(5, 105, apIP.toString().c_str(), ST77XX_GREEN, 2);
  }
  
  if (!server) server = new ESP8266WebServer(80);
  server->on("/", [this]() { handleRoot(); });
  server->on("/server", HTTP_POST, [this]() { handleServerConfig(); });
  server->on("/test", HTTP_GET, [this]() { handleTestServer(); });
  server->on("/status", [this]() { handleStatus(); });
  server->on("/reset", [this]() { handleReset(); });
  server->begin();
  
  // Wait for config (5 min timeout)
  unsigned long startTime = millis();
  while (millis() - startTime < 300000) {
    server->handleClient();
    if (tempServerIP.length() > 0 && tempServerPort > 0) {
      server->stop();
      WiFi.softAPdisconnect(true);
      return true;
    }
    delay(10);
  }
  
  DEBUG_PRINTLN(F("[CFG] Timeout!"));
  return false;
}

bool ConfigManager::startWiFiConfigPortal() {
  DEBUG_PRINTLN(F("\n[CFG] Step 2: WiFi Config"));
  
  // Show WiFi config step on display
  if (displayManager) {
    displayManager->clear();
    displayManager->drawText(10, 15, "WIFI SETUP", ST77XX_YELLOW, 2);
    
    displayManager->drawText(5, 50, "Step 1:", ST77XX_WHITE, 1);
    displayManager->drawText(5, 65, "Reconnect to", ST77XX_WHITE, 1);
    displayManager->drawText(5, 80, apSSID, ST77XX_CYAN, 1);
    
    displayManager->drawText(5, 90, "Step 2:", ST77XX_WHITE, 1);
    displayManager->drawText(5, 105, "Select WiFi", ST77XX_WHITE, 1);
    displayManager->drawText(5, 115, "& Enter Pass", ST77XX_WHITE, 1);
  }
  
  if (!wifiManager) wifiManager = new WiFiManager();
  
  // Disable WiFiManager debug output (reduce serial spam)
  wifiManager->setDebugOutput(false);
  
  wifiManager->resetSettings();
  wifiManager->setConfigPortalTimeout(180);
  wifiManager->setShowInfoUpdate(true);  // Enable OTA update button on info page
  
  if (wifiManager->startConfigPortal(apSSID, apPassword)) {
    DEBUG_PRINT(F("[CFG] WiFi: "));
    DEBUG_PRINT(WiFi.SSID());
    DEBUG_PRINT(F(" IP: "));
    DEBUG_PRINTLN(WiFi.localIP());
    return true;
  }
  
  DEBUG_PRINTLN(F("[CFG] WiFi failed!"));
  return false;
}

void ConfigManager::handleClient() {
  if (server != nullptr && configMode) {
    server->handleClient();
  }
}

// ============= Web Handlers =============

void ConfigManager::handleRoot() {
  server->send(200, "text/html", ConfigPortal::generateServerConfigHTML());
}

void ConfigManager::handleServerConfig() {
  if (server->hasArg("ip")) {
    tempServerIP = server->arg("ip");
    
    // Port is optional - default to 80 if empty or not provided
    if (server->hasArg("port") && server->arg("port").length() > 0) {
      tempServerPort = server->arg("port").toInt();
    } else {
      tempServerPort = 80;  // Default HTTP port
    }
    
    DEBUG_PRINT(F("[CFG] Config: "));
    DEBUG_PRINT(tempServerIP);
    DEBUG_PRINT(F(":"));
    DEBUG_PRINTLN(tempServerPort);
    server->send(200, "text/html", ConfigPortal::generateSuccessHTML(apSSID, tempServerIP, tempServerPort));
    
    // Delay to let user see success page before WiFi portal switch
    delay(2000);
  } else {
    server->send(400, "text/plain", "Missing server address");
  }
}

void ConfigManager::handleTestServer() {
  server->send(200, "text/html", ConfigPortal::generateTestingHTML());
}

void ConfigManager::handleStatus() {
  String j = "{\"serverIP\":\"" + tempServerIP + "\",\"serverPort\":" + String(tempServerPort) + 
             ",\"hasServerConfig\":" + (tempServerIP.length() > 0 ? "true" : "false") + "}";
  server->send(200, "application/json", j);
}

void ConfigManager::handleReset() {
  resetConfig();
  server->send(200, "text/plain", "Reset! Rebooting...");
  delay(2000);
  ESP.restart();
}
