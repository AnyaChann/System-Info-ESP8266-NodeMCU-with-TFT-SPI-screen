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
    tempServerPort(8080), connectionFailCount(0), lastConnectionAttempt(0),
    lastConnectedShown(false), displayManager(nullptr) {
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
}

void ConfigManager::setServerPort(uint16_t port) {
  config.serverPort = port;
}

void ConfigManager::setWiFiCredentials(const char* ssid, const char* pass) {
  strncpy(config.wifiSSID, ssid, sizeof(config.wifiSSID) - 1);
  config.wifiSSID[sizeof(config.wifiSSID) - 1] = '\0';
  
  strncpy(config.wifiPassword, pass, sizeof(config.wifiPassword) - 1);
  config.wifiPassword[sizeof(config.wifiPassword) - 1] = '\0';
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
  // WiFi connected - reset and return
  if (WiFi.status() == WL_CONNECTED) {
    connectionFailCount = 0;
    lastConnectedShown = false; // Reset flag when stable
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
    showReconnectDisplay();
    
    WiFi.disconnect();
    delay(100);
    WiFi.begin(config.wifiSSID, config.wifiPassword);
    
    // Progressive timeout: 10s->30s
    int timeout = min(5000 + (connectionFailCount * 5000), 30000);
    unsigned long startTime = millis();
    
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout) {
      delay(500);
      Serial.print(F("."));
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print(F("✓ Reconnected! IP: "));
      Serial.println(WiFi.localIP());
      
      // Only show "WiFi OK!" once after reconnect
      if (displayManager && !lastConnectedShown) {
        displayManager->showWiFiStatus(true, WiFi.localIP().toString());
        lastConnectedShown = true;
      }
      
      connectionFailCount = 0;
      return false;
    }
    
    Serial.print(F("✗ Failed ("));
    Serial.print(timeout / 1000);
    Serial.println(F("s)"));
    showRetryDisplay();
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

// ============= Config Portal =============

bool ConfigManager::startConfigPortalWithValidation() {
  configMode = true;
  Serial.println(F("\n=== CONFIG PORTAL ==="));
  
  // Step 1: Server config
  if (!startServerConfigPortal()) return false;
  
  // Step 2: WiFi config
  if (!startWiFiConfigPortal()) return false;
  
  // Step 3: Validate server
  Serial.println(F("\nStep 3: Validating..."));
  if (!testServerConnection(tempServerIP.c_str(), tempServerPort, 5000)) {
    Serial.println(F("✗ Validation failed!"));
    WiFi.disconnect();
    return false;
  }
  
  // Step 4: Save
  Serial.println(F("\nStep 4: Saving..."));
  setServerIP(tempServerIP.c_str());
  setServerPort(tempServerPort);
  setWiFiCredentials(WiFi.SSID().c_str(), WiFi.psk().c_str());
  
  if (saveConfig()) {
    Serial.println(F("✓ Saved! Rebooting..."));
    configMode = false;
    delay(3000);
    ESP.restart();
    return true;
  }
  
  Serial.println(F("✗ Save failed!"));
  return false;
}

bool ConfigManager::startServerConfigPortal() {
  Serial.println(F("Step 1: Server Config"));
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID, apPassword);
  Serial.print(F("AP: "));
  Serial.println(WiFi.softAPIP());
  
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
  
  Serial.println(F("Timeout!"));
  return false;
}

bool ConfigManager::startWiFiConfigPortal() {
  Serial.println(F("\nStep 2: WiFi Config"));
  
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
