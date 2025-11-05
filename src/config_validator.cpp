/*
 * Config Validator Implementation
 */

#include "config.h"
#include "config_validator.h"

bool ConfigValidator::testWiFi(const char* ssid, const char* pass, int timeout) {
  DEBUG_PRINTLN(F("\n[VAL] Testing WiFi"));
  DEBUG_PRINT(F("[VAL] SSID: "));
  DEBUG_PRINTLN(ssid);
  
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout) {
    delay(500);
    DEBUG_PRINT(F("."));
  }
  DEBUG_PRINTLN("");
  
  bool connected = (WiFi.status() == WL_CONNECTED);
  if (connected) {
    DEBUG_PRINT(F("[VAL] WiFi OK! IP: "));
    DEBUG_PRINTLN(WiFi.localIP());
  } else {
    DEBUG_PRINTLN(F("[VAL] WiFi failed!"));
  }
  
  return connected;
}

bool ConfigValidator::testServer(const char* serverIP, uint16_t serverPort, int timeout) {
  DEBUG_PRINTLN(F("\n[VAL] Testing Server"));
  DEBUG_PRINTF("[VAL] URL: http://%s:%d/system-info\n", serverIP, serverPort);
  
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINTLN(F("[VAL] WiFi not connected!"));
    return false;
  }
  
  WiFiClient client;
  HTTPClient http;
  String url = String("http://") + serverIP + ":" + serverPort + "/system-info";
  
  http.begin(client, url);
  http.setTimeout(timeout);
  int httpCode = http.GET();
  http.end();
  
  bool success = (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY);
  DEBUG_PRINT(F("[VAL] HTTP: "));
  DEBUG_PRINTLN(httpCode);
  DEBUG_PRINTLN(success ? F("[VAL] Server OK!") : F("[VAL] Server failed!"));
  
  return success;
}
