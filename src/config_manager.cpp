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
    Serial.println(F("No valid config found, starting config portal"));
    startConfigPortalWithValidation();
  } else {
    Serial.println(F("Config loaded successfully"));
    Serial.print(F("Server: "));
    Serial.println(getServerURL());
    Serial.print(F("WiFi: "));
    Serial.println(getWiFiSSID());
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
  Serial.println(F("Config reset!"));
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
  Serial.print(F("Set Server IP: "));
  Serial.println(config.serverIP);
}

void ConfigManager::setServerPort(uint16_t port) {
  config.serverPort = port;
  Serial.print(F("Set Server Port: "));
  Serial.println(config.serverPort);
}

void ConfigManager::setWiFiCredentials(const char* ssid, const char* pass) {
  strncpy(config.wifiSSID, ssid, sizeof(config.wifiSSID) - 1);
  config.wifiSSID[sizeof(config.wifiSSID) - 1] = '\0';
  
  strncpy(config.wifiPassword, pass, sizeof(config.wifiPassword) - 1);
  config.wifiPassword[sizeof(config.wifiPassword) - 1] = '\0';
  
  Serial.print(F("Set WiFi SSID: "));
  Serial.println(config.wifiSSID);
  Serial.print(F("Set WiFi Pass: "));
  Serial.println(config.wifiPassword[0] ? "****" : "(empty)");
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
  Serial.print(F("WiFi fail: "));
  Serial.println(connectionFailCount);
  
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
      Serial.print(F("."));
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print(F("✓ Reconnected! IP: "));
      Serial.println(WiFi.localIP());
      
      // No display - let main loop show system info directly
      connectionFailCount = 0;
      return false;
    }
    
    Serial.print(F("✗ Failed ("));
    Serial.print(timeout / 1000);
    Serial.println(F("s)"));
    if (displayManager) showRetryDisplay();
  }
  
  // Check if reached max fails
  if (connectionFailCount >= 5) {
    Serial.println(F("\n⚠️ TOO MANY FAILURES! Entering config mode...\n"));
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
  
  Serial.print(F("Server fail: "));
  Serial.print(serverFailCount);
  Serial.println(F("/10"));
  
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
    Serial.println(F("\n⚠️ SERVER UNREACHABLE! Entering config mode...\n"));
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
  Serial.println(F("\n=== CONFIG PORTAL ==="));
  
  // Step 1: WiFi config FIRST (so we have network for server config)
  if (!startWiFiConfigPortal()) return false;
  
  // Step 2: Server config (on the WiFi network we just connected to)
  if (!startServerConfigPortal()) return false;
  
  // Step 3: Validate server (optional - save anyway if failed)
  Serial.println(F("\nStep 3: Validating..."));
  bool serverValid = testServerConnection(tempServerIP.c_str(), tempServerPort, 5000);
  
  if (!serverValid) {
    Serial.println(F("⚠️ Warning: Server validation failed!"));
    Serial.println(F("Config will be saved anyway. You can fix server later."));
    
    if (displayManager) {
      displayManager->clear();
      displayManager->drawText(5, 30, "WARNING!", ST77XX_YELLOW, 2);
      displayManager->drawText(5, 60, "Server test", ST77XX_WHITE, 1);
      displayManager->drawText(5, 75, "failed!", ST77XX_WHITE, 1);
      displayManager->drawText(5, 95, "Saving config", ST77XX_CYAN, 1);
      displayManager->drawText(5, 110, "anyway...", ST77XX_CYAN, 1);
      delay(2000);
    }
  } else {
    Serial.println(F("✓ Server validated!"));
  }
  
  // Step 4: Save config (even if server validation failed)
  Serial.println(F("\nStep 4: Saving..."));
  setServerIP(tempServerIP.c_str());
  setServerPort(tempServerPort);
  setWiFiCredentials(WiFi.SSID().c_str(), WiFi.psk().c_str());
  
  if (saveConfig()) {
    Serial.println(F("✓ Config saved!"));
    
    if (displayManager) {
      displayManager->clear();
      displayManager->drawText(5, 50, "SAVED!", ST77XX_GREEN, 2);
      displayManager->drawText(5, 80, "Rebooting...", ST77XX_WHITE, 1);
    }
    
    configMode = false;
    delay(3000);
    ESP.restart();
    return true;
  }
  
  Serial.println(F("✗ Save failed!"));
  
  if (displayManager) {
    displayManager->clear();
    displayManager->drawText(5, 50, "SAVE ERROR!", ST77XX_RED, 2);
    delay(3000);
  }
  
  return false;
}

bool ConfigManager::startServerConfigPortal() {
  Serial.println(F("\nStep 2: Server Config"));
  
  // WiFi already connected from Step 1
  IPAddress localIP = WiFi.localIP();
  
  Serial.print(F("Server config at: "));
  Serial.println(localIP);
  
  // Show server config info on display
  if (displayManager) {
    displayManager->clear();
    displayManager->drawText(5, 15, "STEP 2:", ST77XX_YELLOW, 1);
    displayManager->drawText(5, 35, "Server Setup", ST77XX_WHITE, 2);
    displayManager->drawText(5, 65, "Open browser:", ST77XX_WHITE, 1);
    displayManager->drawText(5, 80, localIP.toString().c_str(), ST77XX_GREEN, 1);
    displayManager->drawText(5, 100, "Enter server", ST77XX_CYAN, 1);
    displayManager->drawText(5, 115, "IP & port", ST77XX_CYAN, 1);
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
      // Don't disconnect WiFi - we're using STA mode now, not AP
      return true;
    }
    delay(10);
  }
  
  Serial.println(F("Timeout!"));
  return false;
}

bool ConfigManager::startWiFiConfigPortal() {
  Serial.println(F("\nStep 1: WiFi Config"));
  
  // Show WiFi config step on display
  if (displayManager) {
    displayManager->clear();
    displayManager->drawText(5, 15, "STEP 1:", ST77XX_YELLOW, 1);
    displayManager->drawText(5, 35, "WiFi Setup", ST77XX_WHITE, 2);
    displayManager->drawText(5, 70, "Connect to:", ST77XX_WHITE, 1);
    displayManager->drawText(5, 85, apSSID, ST77XX_CYAN, 1);
    displayManager->drawText(5, 105, "Select your", ST77XX_WHITE, 1);
    displayManager->drawText(5, 120, "WiFi network", ST77XX_WHITE, 1);
  }
  
  if (!wifiManager) wifiManager = new WiFiManager();
  wifiManager->resetSettings();
  wifiManager->setConfigPortalTimeout(180);
  
  if (wifiManager->startConfigPortal(apSSID, apPassword)) {
    Serial.print(F("✓ WiFi: "));
    Serial.print(WiFi.SSID());
    Serial.print(F(" IP: "));
    Serial.println(WiFi.localIP());
    return true;
  }
  
  Serial.println(F("✗ WiFi failed!"));
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
  if (server->hasArg("ip") && server->hasArg("port")) {
    tempServerIP = server->arg("ip");
    tempServerPort = server->arg("port").toInt();
    Serial.print(F("Config: "));
    Serial.print(tempServerIP);
    Serial.print(F(":"));
    Serial.println(tempServerPort);
    server->send(200, "text/html", ConfigPortal::generateSuccessHTML(apSSID, tempServerIP, tempServerPort));
  } else {
    server->send(400, "text/plain", "Missing parameters");
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
