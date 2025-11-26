/*
 * Config Manager Implementation - Lightweight with modular dependencies
 */

#include "config.h"  // Must include first for TFT type definition
#include "config_manager.h"
#include "config_validator.h"
#include "display_manager.h"
#include "button_handler.h"

// ESP8266 WiFi credentials struct
extern "C" {
  #include "user_interface.h"
}

ConfigManager::ConfigManager(const char* apName, const char* apPass)
  : wifiManager(nullptr), server(nullptr), configMode(false), 
    apSSID(apName), apPassword(apPass),
    tempServerPort(8080), tempWiFiSSID(""), tempWiFiPassword(""),
    connectionFailCount(0), serverFailCount(0),
    lastConnectionAttempt(0), lastServerCheck(0), 
    displayManager(nullptr), buttonHandler(nullptr) {
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
  DEBUG_PRINTLN(F("[CFG] Config reset (ALL)!"));
}

void ConfigManager::resetServerConfig() {
  // Clear server config only, keep WiFi
  memset(config.serverIP, 0, sizeof(config.serverIP));
  config.serverPort = 8080;  // Default
  saveConfig();
  DEBUG_PRINTLN(F("[CFG] Server config reset (WiFi kept)!"));
}

void ConfigManager::resetWiFiConfig() {
  // Clear WiFi config only, keep server
  memset(config.wifiSSID, 0, sizeof(config.wifiSSID));
  memset(config.wifiPassword, 0, sizeof(config.wifiPassword));
  saveConfig();
  DEBUG_PRINTLN(F("[CFG] WiFi config reset (Server kept)!"));
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
    
    // Progressive timeout: 5s->15s (reduced for better UX)
    int timeout = min(5000 + (connectionFailCount * 3000), 15000);
    unsigned long startTime = millis();
    int dots = 0;
    unsigned long lastDisplayUpdate = 0;
    
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < (unsigned long)timeout) {
      unsigned long elapsed = millis() - startTime;
      
      // Update display every 500ms
      if (displayManager && elapsed - lastDisplayUpdate > 500) {
        lastDisplayUpdate = elapsed;
        
        showReconnectDisplay();
        
        // Show remaining time
        int remaining = (timeout - elapsed) / 1000;
        String timeStr = "Timeout: " + String(remaining) + "s";
        displayManager->drawText(10, 130, timeStr + "     ", ST77XX_YELLOW, 1);
        
        // Animated dots
        displayManager->drawText(10, 145, "          ", ST77XX_BLACK, 1); // Clear
        for (int i = 0; i < (dots % 4); i++) {
          displayManager->drawText(10 + (i * 10), 145, ".", ST77XX_YELLOW, 2);
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
  
  // Step 1: Server IP config FIRST (no WiFi needed, simpler!)
  if (!startServerConfigPortal()) return false;
  
  // Step 2: WiFi config SECOND (more stable without AP conflicts)
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
  
  // Debug: Check what we're saving
  DEBUG_PRINT(F("[CFG] Temp WiFi SSID: '"));
  DEBUG_PRINT(tempWiFiSSID);
  DEBUG_PRINTLN(F("'"));
  DEBUG_PRINT(F("[CFG] Temp WiFi Password length: "));
  DEBUG_PRINTLN(tempWiFiPassword.length());
  DEBUG_PRINT(F("[CFG] WiFi connected: "));
  DEBUG_PRINTLN(WiFi.status() == WL_CONNECTED ? "YES" : "NO");
  DEBUG_PRINT(F("[CFG] Server IP: '"));
  DEBUG_PRINT(tempServerIP);
  DEBUG_PRINTLN(F("'"));
  
  // Validation: SSID must not be empty
  if (tempWiFiSSID.length() == 0) {
    DEBUG_PRINTLN(F("[CFG] ERROR: WiFi SSID is empty!"));
    if (displayManager) {
      displayManager->clear();
      displayManager->drawText(10, 50, "ERROR!", ST77XX_RED, 2);
      displayManager->drawText(5, 80, "WiFi lost!", ST77XX_WHITE, 1);
      delay(3000);
    }
    return false;
  }
  
  // Save BOTH SSID and password to EEPROM
  // This ensures config persists even if ESP flash gets cleared
  DEBUG_PRINTLN(F("[CFG] Saving WiFi credentials to EEPROM"));
  DEBUG_PRINT(F("[CFG] SSID: "));
  DEBUG_PRINTLN(tempWiFiSSID);
  DEBUG_PRINT(F("[CFG] Password length: "));
  DEBUG_PRINTLN(tempWiFiPassword.length());
  
  setWiFiCredentials(tempWiFiSSID.c_str(), tempWiFiPassword.c_str());
  
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
  DEBUG_PRINTLN(F("[CFG] Step 2: Server Config"));
  
  // Keep current WiFi connection if exists, otherwise start AP
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_AP);
  } else {
    WiFi.mode(WIFI_AP_STA);  // Both AP and STA mode
  }
  WiFi.softAP(apSSID, apPassword);
  IPAddress apIP = WiFi.softAPIP();
  
  DEBUG_PRINT(F("[CFG] AP: "));
  DEBUG_PRINTLN(apIP);
  
  // Show AP info on display (WiFi already connected!)
  if (displayManager) {
    displayManager->clear();
    displayManager->drawText(10, 10, "SERVER SETUP", ST77XX_YELLOW, 2);
    
    // Show connected WiFi info
    displayManager->drawText(5, 40, "WiFi OK:", ST77XX_GREEN, 1);
    displayManager->drawText(5, 55, WiFi.SSID().c_str(), ST77XX_CYAN, 1);
    displayManager->drawText(5, 70, WiFi.localIP().toString().c_str(), ST77XX_WHITE, 1);
    
    displayManager->drawText(5, 90, "Config AP:", ST77XX_WHITE, 1);
    displayManager->drawText(5, 105, apSSID, ST77XX_YELLOW, 1);
    
    displayManager->drawText(5, 125, "Browse:", ST77XX_WHITE, 1);
    displayManager->drawText(5, 140, apIP.toString().c_str(), ST77XX_GREEN, 1);
    
    // Timeout info
    displayManager->drawText(5, 160, "Timeout: 3min", ST77XX_YELLOW, 1);
  }
  
  if (!server) server = new ESP8266WebServer(80);
  server->on("/", [this]() { handleRoot(); });
  server->on("/server", HTTP_POST, [this]() { handleServerConfig(); });
  server->on("/cancel", HTTP_POST, [this]() { handleCancel(); });
  server->on("/test", HTTP_GET, [this]() { handleTestServer(); });
  server->on("/status", [this]() { handleStatus(); });
  server->on("/reset", [this]() { handleReset(); });
  server->begin();
  
  // Wait for config with timeout (3 min)
  const unsigned long TIMEOUT = 180000;  // 3 minutes
  unsigned long startTime = millis();
  unsigned long lastTimeoutUpdate = 0;
  
  // Track button press for exit
  unsigned long buttonPressStart = 0;
  bool wasPressed = false;
  
  while (millis() - startTime < TIMEOUT) {
    server->handleClient();
    
    // Check button for long press (7s) to exit
    if (buttonHandler) {
      buttonHandler->update();
      
      bool isPressed = buttonHandler->isPressed();
      
      // Button just pressed - start timer
      if (isPressed && !wasPressed) {
        buttonPressStart = millis();
        wasPressed = true;
      }
      // Button still pressed - check duration
      else if (isPressed && wasPressed) {
        unsigned long pressDuration = millis() - buttonPressStart;
        
        // Show countdown every second
        if (pressDuration > 0 && pressDuration % 1000 < 100 && displayManager) {
          int secondsRemaining = 7 - (pressDuration / 1000);
          if (secondsRemaining > 0 && secondsRemaining <= 7) {
            String countStr = "Hold: " + String(secondsRemaining) + "s";
            displayManager->drawText(15, 160, countStr + "   ", ST77XX_YELLOW, 1);
          }
        }
        
        // Exit after 7 seconds
        if (pressDuration >= 7000) {
          DEBUG_PRINTLN(F("[CFG] Long press detected - Exiting..."));
          if (displayManager) {
            displayManager->clear();
            displayManager->drawText(20, 60, "CANCELLED", ST77XX_RED, 2);
            displayManager->drawText(30, 95, "Restarting", ST77XX_WHITE, 1);
          }
          server->stop();
          delay(2000);
          ESP.restart();
          return false;
        }
      }
      // Button released - reset
      else if (!isPressed && wasPressed) {
        wasPressed = false;
        // Clear countdown text
        if (displayManager) {
          displayManager->drawText(15, 160, "           ", ST77XX_BLACK, 1);
        }
      }
    }
    
    // Check if config completed
    if (tempServerIP.length() > 0 && tempServerPort > 0) {
      server->stop();
      WiFi.softAPdisconnect(true);
      return true;
    }
    
    // Show remaining time every 30 seconds
    unsigned long elapsed = millis() - startTime;
    if (elapsed - lastTimeoutUpdate > 30000 && displayManager) {
      lastTimeoutUpdate = elapsed;
      int remainingMin = (TIMEOUT - elapsed) / 60000;
      int remainingSec = ((TIMEOUT - elapsed) % 60000) / 1000;
      
      String timeStr = "Timeout: " + String(remainingMin) + "m " + String(remainingSec) + "s";
      displayManager->drawText(5, 160, timeStr + "     ", ST77XX_YELLOW, 1);
      
      DEBUG_PRINT(F("[CFG] Remaining: "));
      DEBUG_PRINT(remainingMin);
      DEBUG_PRINT(F("m "));
      DEBUG_PRINT(remainingSec);
      DEBUG_PRINTLN(F("s"));
    }
    
    delay(10);
  }
  
  // Timeout occurred
  DEBUG_PRINTLN(F("[CFG] Server config timeout!"));
  if (displayManager) {
    displayManager->clear();
    displayManager->drawText(20, 60, "TIMEOUT!", ST77XX_RED, 2);
    displayManager->drawText(30, 95, "Restarting", ST77XX_WHITE, 1);
  }
  server->stop();
  delay(3000);
  ESP.restart();
  return false;
}

bool ConfigManager::startWiFiConfigPortal() {
  DEBUG_PRINTLN(F("\n[CFG] Step 1: WiFi Config"));
  
  // Show WiFi config step on display
  if (displayManager) {
    displayManager->clear();
    displayManager->drawText(10, 15, "WIFI SETUP", ST77XX_YELLOW, 2);
    displayManager->drawText(5, 45, "Connect to:", ST77XX_WHITE, 1);
    
    displayManager->drawText(5, 65, "SSID:", ST77XX_CYAN, 1);
    displayManager->drawText(5, 80, apSSID, ST77XX_YELLOW, 1);
    
    displayManager->drawText(5, 100, "Password:", ST77XX_CYAN, 1);
    displayManager->drawText(5, 115, apPassword, ST77XX_YELLOW, 1);
    
    displayManager->drawText(5, 135, "Then config WiFi", ST77XX_WHITE, 1);
    displayManager->drawText(5, 145, "in browser", ST77XX_WHITE, 1);
    
    // Timeout info and button hint
    displayManager->drawText(5, 160, "Hold BTN 7s to", ST77XX_YELLOW, 1);
    displayManager->drawText(5, 173, "exit (timeout 3m)", ST77XX_YELLOW, 1);
  }
  
  if (!wifiManager) wifiManager = new WiFiManager();
  
  // Disable WiFiManager debug output (reduce serial spam)
  wifiManager->setDebugOutput(false);
  
  // First, disconnect and clear WiFiManager's data (NOT ESP WiFi credentials)
  // Complete WiFi reset sequence to fix SSID corruption
  WiFi.persistent(false);      // Don't save during cleanup
  WiFi.disconnect(true);       // Disconnect and erase STA config
  WiFi.softAPdisconnect(true); // Disconnect and erase AP config
  WiFi.mode(WIFI_OFF);         // Turn off WiFi completely
  delay(1000);                 // Longer delay for complete shutdown
  
  // Reset WiFiManager settings (clears WiFiManager's stored data)
  wifiManager->resetSettings();
  
  wifiManager->setConfigPortalTimeout(180);
  wifiManager->setShowInfoUpdate(true);  // Enable OTA update button on info page
  
  // Add Exit button to WiFi portal (WiFiManager already has /exit endpoint)
  String customHtml = F("<style>.c{text-align:center}.btn{padding:12px;border:none;border-radius:8px;font-size:15px;width:100%;margin:8px 0;cursor:pointer;font-weight:600;transition:transform 0.2s}");
  customHtml += F(".btn-primary{background:linear-gradient(135deg,#667eea,#764ba2);color:white}");
  customHtml += F(".btn-secondary{background:#6c757d;color:white}.btn:hover{transform:translateY(-2px)}</style>");
  customHtml += F("<br/><form action='/exit' method='get'><button class='btn btn-secondary'>Cancel & Restart</button></form>");
  wifiManager->setCustomHeadElement(customHtml.c_str());
  
  // Enable WiFi persistent mode - ESP will auto-save credentials
  WiFi.persistent(true);
  WiFi.setAutoConnect(false);  // Don't auto-connect during config portal
  WiFi.setAutoReconnect(true);
  
  DEBUG_PRINT(F("[CFG] Starting portal with SSID: "));
  DEBUG_PRINTLN(apSSID);
  DEBUG_PRINT(F("[CFG] Password: "));
  DEBUG_PRINTLN(apPassword);
  
  if (wifiManager->startConfigPortal(apSSID, apPassword)) {
    // Verify WiFi actually connected successfully
    DEBUG_PRINTLN(F("[CFG] Verifying WiFi connection..."));
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
      delay(500);
      attempts++;
      DEBUG_PRINT(F("."));
    }
    DEBUG_PRINTLN();
    
    if (WiFi.status() != WL_CONNECTED) {
      DEBUG_PRINTLN(F("[CFG] ERROR: WiFi not connected after portal!"));
      DEBUG_PRINTLN(F("[CFG] Password may be incorrect"));
      
      // Clear the bad credentials from ESP flash
      WiFi.persistent(true);
      WiFi.disconnect(true);  // Erase saved credentials
      delay(100);
      
      return false;  // Force user to retry
    }
    
    // Store WiFi credentials ONLY after successful connection
    tempWiFiSSID = WiFi.SSID();
    
    DEBUG_PRINT(F("[CFG] WiFi Connected: "));
    DEBUG_PRINT(tempWiFiSSID);
    DEBUG_PRINT(F(" IP: "));
    DEBUG_PRINTLN(WiFi.localIP());
    
    // ALWAYS get password from station_config (most reliable)
    struct station_config conf;
    wifi_station_get_config(&conf);
    conf.password[63] = '\0';  // Ensure null-terminated
    
    // Validate password is printable ASCII
    bool valid = true;
    for (size_t i = 0; i < strlen((char*)conf.password); i++) {
      if (conf.password[i] < 32 || conf.password[i] > 126) {
        valid = false;
        break;
      }
    }
    
    if (valid && strlen((char*)conf.password) > 0) {
      tempWiFiPassword = String(reinterpret_cast<char*>(conf.password));
      DEBUG_PRINT(F("[CFG] Got password from station_config, length: "));
      DEBUG_PRINTLN(tempWiFiPassword.length());
    } else {
      DEBUG_PRINTLN(F("[CFG] WARNING: Could not get valid password!"));
      DEBUG_PRINTLN(F("[CFG] Will save to EEPROM anyway for ESP flash fallback"));
      tempWiFiPassword = "";  // Empty - rely on ESP flash
    }
    
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

void ConfigManager::handleCancel() {
  DEBUG_PRINTLN(F("[CFG] User cancelled config"));
  
  String html = F("<!DOCTYPE html><html><head><meta charset='UTF-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>Cancelled</title>");
  html += F("<style>");
  html += F("body{font-family:sans-serif;background:#667eea;color:white;text-align:center;padding:50px}");
  html += F("h1{font-size:24px;margin-bottom:16px}");
  html += F("p{opacity:0.9}");
  html += F("</style></head><body>");
  html += F("<h1>Configuration Cancelled</h1>");
  html += F("<p>Restarting device...</p>");
  html += F("</body></html>");
  
  server->send(200, "text/html", html);
  delay(2000);
  ESP.restart();
}

void ConfigManager::handleReset() {
  resetConfig();
  server->send(200, "text/plain", "Reset! Rebooting...");
  delay(2000);
  ESP.restart();
}
