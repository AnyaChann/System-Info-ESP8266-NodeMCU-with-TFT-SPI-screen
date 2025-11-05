/*
 * Config Validator Implementation
 */

#include "config_validator.h"

bool ConfigValidator::testWiFi(const char* ssid, const char* pass, int timeout) {
  Serial.println(F("\n--- Testing WiFi ---"));
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
    Serial.print(F("✓ WiFi OK! IP: "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("✗ WiFi failed!"));
  }
  
  return connected;
}

bool ConfigValidator::testServer(const char* serverIP, uint16_t serverPort, int timeout) {
  Serial.println(F("\n--- Testing Server ---"));
  Serial.print(F("URL: http://"));
  Serial.print(serverIP);
  Serial.print(F(":"));
  Serial.print(serverPort);
  Serial.println(F("/system-info"));
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("✗ WiFi not connected!"));
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
  Serial.print(F("HTTP: "));
  Serial.println(httpCode);
  Serial.println(success ? F("✓ Server OK!") : F("✗ Server failed!"));
  
  return success;
}
