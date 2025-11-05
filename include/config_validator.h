/*
 * Config Validator - WiFi and Server connection testing
 */

#ifndef CONFIG_VALIDATOR_H
#define CONFIG_VALIDATOR_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

class ConfigValidator {
public:
  // Test WiFi connection
  static bool testWiFi(const char* ssid, const char* pass, int timeout = 15000);
  
  // Test server connection
  static bool testServer(const char* serverIP, uint16_t serverPort, int timeout = 5000);
};

#endif // CONFIG_VALIDATOR_H
