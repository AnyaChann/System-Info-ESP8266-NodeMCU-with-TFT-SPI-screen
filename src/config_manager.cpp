/*
 * Config Manager Implementation with WiFiManager and Validation
 */

#include "config.h"  // Must include first for TFT type definition
#include "config_manager.h"
#include "display_manager.h"

ConfigManager::ConfigManager(const char* apName, const char* apPass)
  : wifiManager(nullptr), server(nullptr), configMode(false), 
    apSSID(apName), apPassword(apPass),
    tempServerPort(8080), connectionFailCount(0), lastConnectionAttempt(0),
    displayManager(nullptr) {
  clearConfig();
}

void ConfigManager::begin() {
  EEPROM.begin(EEPROM_SIZE);
  
  // Try to load saved config
  if (!loadConfig()) {
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
  EEPROM.get(0, config);
  
  Serial.println(F("\n--- Loading Config from EEPROM ---"));
  Serial.print(F("Magic: 0x"));
  Serial.print(config.magic, HEX);
  Serial.print(F(" (expected: 0x"));
  Serial.print(EEPROM_MAGIC, HEX);
  Serial.println(F(")"));
  Serial.print(F("Version: "));
  Serial.println(config.version);
  
  // Verify magic and version
  if (config.magic != EEPROM_MAGIC || config.version != EEPROM_VERSION) {
    Serial.println(F("✗ Invalid magic or version"));
    return false;
  }
  
  // Verify checksum
  uint8_t expectedChecksum = calculateChecksum();
  Serial.print(F("Checksum: 0x"));
  Serial.print(config.checksum, HEX);
  Serial.print(F(" (expected: 0x"));
  Serial.print(expectedChecksum, HEX);
  Serial.println(F(")"));
  
  if (!verifyChecksum()) {
    Serial.println(F("✗ Checksum mismatch!"));
    return false;
  }
  
  // Check if required fields are filled
  Serial.print(F("Server IP: "));
  Serial.println(config.serverIP);
  Serial.print(F("Server Port: "));
  Serial.println(config.serverPort);
  Serial.print(F("WiFi SSID: "));
  Serial.println(config.wifiSSID);
  
  if (strlen(config.serverIP) == 0 || strlen(config.wifiSSID) == 0) {
    Serial.println(F("✗ Required fields empty"));
    return false;
  }
  
  Serial.println(F("✓ Config loaded successfully!"));
  return true;
}

bool ConfigManager::saveConfig() {
  config.magic = EEPROM_MAGIC;
  config.version = EEPROM_VERSION;
  config.checksum = calculateChecksum();
  
  Serial.println(F("\n--- Saving Config to EEPROM ---"));
  Serial.print(F("Magic: 0x"));
  Serial.println(config.magic, HEX);
  Serial.print(F("Version: "));
  Serial.println(config.version);
  Serial.print(F("Server: "));
  Serial.print(config.serverIP);
  Serial.print(F(":"));
  Serial.println(config.serverPort);
  Serial.print(F("WiFi: "));
  Serial.print(config.wifiSSID);
  Serial.println(F(" / ******"));
  Serial.print(F("Checksum: 0x"));
  Serial.println(config.checksum, HEX);
  Serial.print(F("Total size: "));
  Serial.print(sizeof(ConfigData));
  Serial.println(F(" bytes"));
  
  EEPROM.put(0, config);
  bool success = EEPROM.commit();
  
  if (success) {
    Serial.println(F("✓ Config saved to EEPROM"));
    
    // Verify by reading back
    ConfigData verify;
    EEPROM.get(0, verify);
    Serial.println(F("--- Verifying saved data ---"));
    Serial.print(F("Magic match: "));
    Serial.println(verify.magic == EEPROM_MAGIC ? "YES" : "NO");
    Serial.print(F("Server IP match: "));
    Serial.println(strcmp(verify.serverIP, config.serverIP) == 0 ? "YES" : "NO");
    Serial.print(F("SSID match: "));
    Serial.println(strcmp(verify.wifiSSID, config.wifiSSID) == 0 ? "YES" : "NO");
  } else {
    Serial.println(F("✗ Failed to save config"));
  }
  
  return success;
}

void ConfigManager::resetConfig() {
  clearConfig();
  saveConfig();
  Serial.println(F("Config reset!"));
}

void ConfigManager::clearConfig() {
  memset(&config, 0, sizeof(ConfigData));
  config.serverPort = 8080; // Default port
}

uint8_t ConfigManager::calculateChecksum() {
  uint8_t sum = 0;
  uint8_t* data = (uint8_t*)&config;
  // Calculate checksum excluding the checksum field itself (last byte)
  size_t checksumOffset = offsetof(ConfigData, checksum);
  for (size_t i = 0; i < checksumOffset; i++) {
    sum ^= data[i];
  }
  return sum;
}

bool ConfigManager::verifyChecksum() {
  return config.checksum == calculateChecksum();
}

bool ConfigManager::hasValidConfig() {
  return strlen(config.serverIP) > 0 && strlen(config.wifiSSID) > 0;
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

// ============= WiFi Validation =============

bool ConfigManager::testWiFiConnection(const char* ssid, const char* pass, int timeout) {
  Serial.println(F("\n--- Testing WiFi Connection ---"));
  Serial.print(F("SSID: "));
  Serial.println(ssid);
  
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();
  
  bool connected = (WiFi.status() == WL_CONNECTED);
  
  if (connected) {
    Serial.print(F("✓ WiFi connected! IP: "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("✗ WiFi connection failed!"));
    Serial.print(F("Status: "));
    Serial.println(WiFi.status());
  }
  
  return connected;
}

// ============= Server Validation =============

bool ConfigManager::testServerConnection(const char* serverIP, uint16_t serverPort, int timeout) {
  Serial.println(F("\n--- Testing Server Connection ---"));
  Serial.print(F("Server: "));
  Serial.print(serverIP);
  Serial.print(F(":"));
  Serial.println(serverPort);
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("✗ WiFi not connected!"));
    return false;
  }
  
  WiFiClient client;
  HTTPClient http;
  
  String url = String("http://") + serverIP + ":" + serverPort + "/system-info";
  Serial.print(F("Testing URL: "));
  Serial.println(url);
  
  http.begin(client, url);
  http.setTimeout(timeout);
  
  int httpCode = http.GET();
  bool success = (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY);
  
  Serial.print(F("HTTP Code: "));
  Serial.println(httpCode);
  
  if (success) {
    Serial.println(F("✓ Server responding!"));
  } else {
    Serial.println(F("✗ Server not responding!"));
  }
  
  http.end();
  return success;
}

// ============= Fallback Detection =============

bool ConfigManager::shouldFallbackToConfig() {
  unsigned long currentTime = millis();
  
  // Check if WiFi is connected
  if (WiFi.status() != WL_CONNECTED) {
    // Only count as fail if we haven't checked recently (debounce)
    if (currentTime - lastConnectionAttempt < 10000) {
      // Less than 10 seconds since last check, don't increment
      return false;
    }
    
    connectionFailCount++;
    lastConnectionAttempt = currentTime;
    
    Serial.print(F("WiFi disconnected! Fail count: "));
    Serial.println(connectionFailCount);
    
    // Try to reconnect with progressive timeout
    if (strlen(config.wifiSSID) > 0) {
      Serial.println(F("Attempting to reconnect..."));
      
      // Show simple reconnecting status (only once, no animation during wait)
      if (displayManager != nullptr && connectionFailCount == 1) {
        // Only show on first fail to avoid clearing system info repeatedly
        displayManager->clear();
        displayManager->drawText(10, 50, "WiFi Lost!", ST77XX_RED, 2);
        displayManager->drawText(10, 80, "Reconnecting", ST77XX_YELLOW, 1);
      }
      
      WiFi.disconnect();
      delay(100);
      WiFi.begin(config.wifiSSID, config.wifiPassword);
      
      // Progressive timeout: 10s, 15s, 20s, 25s, 30s
      int timeout = 5000 + (connectionFailCount * 5000);
      if (timeout > 30000) timeout = 30000; // Cap at 30s
      
      unsigned long startTime = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout) {
        delay(500);
        Serial.print(F("."));
        // No display updates during wait - too slow!
      }
      Serial.println();
      
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println(F("✓ Reconnected successfully!"));
        Serial.print(F("IP: "));
        Serial.println(WiFi.localIP());
        
        // Show brief success message (non-blocking)
        if (displayManager != nullptr) {
          displayManager->showWiFiStatus(true, WiFi.localIP().toString());
        }
        
        connectionFailCount = 0;
        return false;
      } else {
        Serial.print(F("✗ Reconnect failed (timeout: "));
        Serial.print(timeout / 1000);
        Serial.println(F("s)"));
        
        // Show brief failure message (non-blocking)
        if (displayManager != nullptr) {
          displayManager->clear();
          displayManager->drawText(5, 50, "Retry...", ST77XX_YELLOW, 2);
          char attemptText[32];
          snprintf(attemptText, sizeof(attemptText), "%d/5", connectionFailCount);
          displayManager->drawText(45, 80, attemptText, ST77XX_WHITE, 2);
        }
      }
    }
    
    // After 5 consecutive fails, go to config mode
    if (connectionFailCount >= 5) {
      Serial.println(F("\n⚠️ TOO MANY CONNECTION FAILURES!"));
      Serial.println(F("Possible reasons:"));
      Serial.println(F("  - Wrong WiFi password"));
      Serial.println(F("  - WiFi router turned off"));
      Serial.println(F("  - Out of range"));
      Serial.println(F("Entering config mode for reconfiguration...\n"));
      return true;
    }
    
    return false; // Still retrying
  }
  
  // WiFi connected - reset fail count
  connectionFailCount = 0;
  
  // DON'T check server in this function - too expensive!
  // Server check should be done separately in loop() with proper timing
  
  return false;
}

// ============= Config Portal with WiFiManager =============

bool ConfigManager::startConfigPortalWithValidation() {
  configMode = true;
  
  Serial.println(F("\n=== CONFIG PORTAL WITH WIFIMANAGER ==="));
  
  // Step 1: Server config via custom web server
  Serial.println(F("Step 1: Configure Server IP"));
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID, apPassword);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print(F("AP IP: "));
  Serial.println(IP);
  Serial.println(F("Connect to WiFi and open: http://192.168.4.1"));
  
  // Start web server for server config
  if (server == nullptr) {
    server = new ESP8266WebServer(80);
  }
  
  server->on("/", [this]() { handleRoot(); });
  server->on("/server", HTTP_POST, [this]() { handleServerConfig(); });
  server->on("/test", HTTP_GET, [this]() { handleTestServer(); });
  server->on("/status", [this]() { handleStatus(); });
  server->on("/reset", [this]() { handleReset(); });
  
  server->begin();
  Serial.println(F("Web server started"));
  
  // Wait for server config
  bool serverConfigured = false;
  unsigned long startTime = millis();
  while (!serverConfigured && millis() - startTime < 300000) { // 5 min timeout
    server->handleClient();
    
    if (strlen(tempServerIP.c_str()) > 0 && tempServerPort > 0) {
      serverConfigured = true;
    }
    delay(10);
  }
  
  if (!serverConfigured) {
    Serial.println(F("Timeout waiting for server config"));
    return false;
  }
  
  server->stop();
  WiFi.softAPdisconnect(true);
  
  // Step 2: WiFi config with WiFiManager
  Serial.println(F("\nStep 2: Configure WiFi with WiFiManager"));
  
  if (wifiManager == nullptr) {
    wifiManager = new WiFiManager();
  }
  
  // Reset settings for fresh start
  wifiManager->resetSettings();
  
  // Set callbacks
  wifiManager->setAPCallback([this](WiFiManager *myWiFiManager) {
    Serial.println(F("Entered config mode"));
    Serial.println(WiFi.softAPIP());
  });
  
  wifiManager->setSaveConfigCallback([this]() {
    Serial.println(F("WiFi config saved callback"));
  });
  
  // Set custom AP name
  wifiManager->setConfigPortalTimeout(180); // 3 minutes timeout
  
  // Start portal and wait for connection
  Serial.println(F("Starting WiFiManager portal..."));
  if (!wifiManager->startConfigPortal(apSSID, apPassword)) {
    Serial.println(F("Failed to connect via WiFiManager"));
    return false;
  }
  
  // WiFi connected!
  Serial.println(F("✓ WiFi connected!"));
  Serial.print(F("IP: "));
  Serial.println(WiFi.localIP());
  Serial.print(F("SSID: "));
  Serial.println(WiFi.SSID());
  
  // Step 3: Validate server connection
  Serial.println(F("\nStep 3: Validating server connection..."));
  
  if (!testServerConnection(tempServerIP.c_str(), tempServerPort, 5000)) {
    Serial.println(F("✗ Server validation failed!"));
    Serial.println(F("Config aborted. Please check server and try again."));
    
    // Show error and restart portal
    WiFi.disconnect();
    return false;
  }
  
  // Step 4: Save validated config
  Serial.println(F("\nStep 4: Saving validated config..."));
  
  setServerIP(tempServerIP.c_str());
  setServerPort(tempServerPort);
  setWiFiCredentials(WiFi.SSID().c_str(), WiFi.psk().c_str());
  
  if (saveConfig()) {
    Serial.println(F("✓ Config saved successfully!"));
    configMode = false;
    
    Serial.println(F("\nRebooting in 3 seconds..."));
    delay(3000);
    ESP.restart();
    return true;
  } else {
    Serial.println(F("✗ Failed to save config!"));
    return false;
  }
}

void ConfigManager::handleClient() {
  if (server != nullptr && configMode) {
    server->handleClient();
  }
}

// ============= Web Handlers =============

void ConfigManager::handleRoot() {
  String html = generateServerConfigHTML();
  server->send(200, "text/html", html);
}

void ConfigManager::handleServerConfig() {
  if (server->hasArg("ip") && server->hasArg("port")) {
    tempServerIP = server->arg("ip");
    tempServerPort = server->arg("port").toInt();
    
    Serial.print(F("Server config received: "));
    Serial.print(tempServerIP);
    Serial.print(F(":"));
    Serial.println(tempServerPort);
    
    // Show success and instructions
    String html = generateSuccessHTML();
    server->send(200, "text/html", html);
  } else {
    server->send(400, "text/plain", "Missing parameters");
  }
}

void ConfigManager::handleTestServer() {
  String html = generateTestingHTML();
  server->send(200, "text/html", html);
}

void ConfigManager::handleStatus() {
  String json = "{";
  json += "\"serverIP\":\"" + tempServerIP + "\",";
  json += "\"serverPort\":" + String(tempServerPort) + ",";
  json += "\"hasServerConfig\":" + String(tempServerIP.length() > 0 ? "true" : "false");
  json += "}";
  server->send(200, "application/json", json);
}

void ConfigManager::handleReset() {
  resetConfig();
  server->send(200, "text/plain", "Config reset! Rebooting...");
  delay(2000);
  ESP.restart();
}

// ============= HTML Generators =============

String ConfigManager::generateServerConfigHTML() {
  String html = F("<!DOCTYPE html><html><head><meta charset='UTF-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>Server Config</title>");
  html += F("<style>");
  html += F("body{font-family:Arial;margin:20px;background:#f0f0f0}");
  html += F(".container{max-width:500px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}");
  html += F("h1{color:#333;text-align:center}");
  html += F(".step{background:#fff3cd;padding:10px;border-left:4px solid #ffc107;margin:10px 0}");
  html += F(".form-group{margin:15px 0}");
  html += F("label{display:block;margin-bottom:5px;color:#555;font-weight:bold}");
  html += F("input{width:100%;padding:10px;border:1px solid #ddd;border-radius:5px;box-sizing:border-box}");
  html += F("button{background:#4CAF50;color:white;padding:12px 20px;border:none;border-radius:5px;cursor:pointer;width:100%;font-size:16px;margin-top:10px}");
  html += F("button:hover{background:#45a049}");
  html += F(".info{background:#e7f3ff;padding:10px;border-left:4px solid #2196F3;margin:15px 0;font-size:14px}");
  html += F("</style></head><body>");
  
  html += F("<div class='container'>");
  html += F("<h1>🔧 ESP8266 Config</h1>");
  
  html += F("<div class='info'>");
  html += F("<strong>📋 Bước 1/2: Server Config</strong><br>");
  html += F("Nhập IP và Port của Python server.<br>");
  html += F("Sau đó sẽ kết nối WiFi với WiFiManager.");
  html += F("</div>");
  
  html += F("<form action='/server' method='POST'>");
  html += F("<div class='form-group'>");
  html += F("<label>🖥️ Server IP:</label>");
  html += F("<input type='text' name='ip' placeholder='192.168.2.60' required pattern='\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}'>");
  html += F("</div>");
  html += F("<div class='form-group'>");
  html += F("<label>🔌 Port:</label>");
  html += F("<input type='number' name='port' placeholder='8080' value='8080' required min='1' max='65535'>");
  html += F("</div>");
  html += F("<button type='submit'>Tiếp theo: Config WiFi ➡️</button>");
  html += F("</form>");
  
  html += F("</div></body></html>");
  return html;
}

String ConfigManager::generateSuccessHTML() {
  String html = F("<!DOCTYPE html><html><head><meta charset='UTF-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>WiFi Config</title>");
  html += F("<style>");
  html += F("body{font-family:Arial;margin:20px;background:#f0f0f0;text-align:center;padding-top:50px}");
  html += F(".container{max-width:500px;margin:0 auto;background:white;padding:40px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}");
  html += F("h1{color:#28a745}");
  html += F(".success{background:#d4edda;padding:15px;border-radius:5px;margin:20px 0}");
  html += F(".step{background:#fff3cd;padding:15px;border-radius:5px;margin:20px 0}");
  html += F("</style></head><body>");
  
  html += F("<div class='container'>");
  html += F("<h1>✅ Bước 1 Hoàn thành!</h1>");
  
  html += F("<div class='success'>");
  html += F("<strong>Server đã config:</strong><br>");
  html += tempServerIP + ":" + String(tempServerPort);
  html += F("</div>");
  
  html += F("<div class='step'>");
  html += F("<strong>📱 Bước 2: Config WiFi</strong><br><br>");
  html += F("1️⃣ ESP8266 sẽ reboot vào WiFiManager mode<br>");
  html += F("2️⃣ Kết nối lại WiFi: <strong>");
  html += String(apSSID);
  html += F("</strong><br>");
  html += F("3️⃣ Chọn WiFi từ list và nhập password<br>");
  html += F("4️⃣ Hệ thống sẽ tự động validate và lưu config<br>");
  html += F("</div>");
  
  html += F("<p>⏳ Đang chuyển sang WiFiManager...</p>");
  html += F("<p style='font-size:12px;color:#666'>Trang này sẽ tự đóng sau 5 giây</p>");
  html += F("</div>");
  
  html += F("<script>setTimeout(function(){window.close()},5000);</script>");
  html += F("</body></html>");
  return html;
}

String ConfigManager::generateTestingHTML() {
  String html = F("<!DOCTYPE html><html><head><meta charset='UTF-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>Testing</title>");
  html += F("</head><body>");
  html += F("<h1>Testing Configuration...</h1>");
  html += F("<p>Please wait...</p>");
  html += F("</body></html>");
  return html;
}

String ConfigManager::generateErrorHTML(const char* error) {
  String html = F("<!DOCTYPE html><html><head><meta charset='UTF-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>Error</title>");
  html += F("<style>");
  html += F("body{font-family:Arial;margin:20px;background:#f0f0f0;text-align:center;padding-top:50px}");
  html += F(".container{max-width:500px;margin:0 auto;background:white;padding:40px;border-radius:10px}");
  html += F(".error{background:#f8d7da;padding:15px;border-radius:5px;color:#721c24}");
  html += F("</style></head><body>");
  html += F("<div class='container'>");
  html += F("<h1>❌ Lỗi</h1>");
  html += F("<div class='error'>");
  html += String(error);
  html += F("</div>");
  html += F("<p><a href='/'>← Thử lại</a></p>");
  html += F("</div></body></html>");
  return html;
}
