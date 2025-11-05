/*
 * Config Storage Implementation
 */

#include "config_storage.h"
#include <string.h>

ConfigStorage::ConfigStorage() {
  EEPROM.begin(EEPROM_SIZE);
}

bool ConfigStorage::load(ConfigData& config) {
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
  uint8_t expectedChecksum = calculateChecksum(config);
  Serial.print(F("Checksum: 0x"));
  Serial.print(config.checksum, HEX);
  Serial.print(F(" (expected: 0x"));
  Serial.print(expectedChecksum, HEX);
  Serial.println(F(")"));
  
  if (!verifyChecksum(config)) {
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

bool ConfigStorage::save(const ConfigData& config) {
  ConfigData tempConfig = config;
  tempConfig.magic = EEPROM_MAGIC;
  tempConfig.version = EEPROM_VERSION;
  tempConfig.checksum = calculateChecksum(tempConfig);
  
  Serial.println(F("\n--- Saving Config to EEPROM ---"));
  Serial.print(F("Magic: 0x"));
  Serial.println(tempConfig.magic, HEX);
  Serial.print(F("Version: "));
  Serial.println(tempConfig.version);
  Serial.print(F("Server: "));
  Serial.print(tempConfig.serverIP);
  Serial.print(F(":"));
  Serial.println(tempConfig.serverPort);
  Serial.print(F("WiFi: "));
  Serial.print(tempConfig.wifiSSID);
  Serial.println(F(" / ******"));
  Serial.print(F("Checksum: 0x"));
  Serial.println(tempConfig.checksum, HEX);
  
  EEPROM.put(0, tempConfig);
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
    Serial.println(strcmp(verify.serverIP, tempConfig.serverIP) == 0 ? "YES" : "NO");
  } else {
    Serial.println(F("✗ Failed to save config"));
  }
  
  return success;
}

void ConfigStorage::clear(ConfigData& config) {
  memset(&config, 0, sizeof(ConfigData));
  config.serverPort = 8080; // Default port
}

uint8_t ConfigStorage::calculateChecksum(const ConfigData& config) {
  uint8_t sum = 0;
  const uint8_t* data = (const uint8_t*)&config;
  // Calculate checksum excluding the checksum field itself (last byte)
  size_t checksumOffset = offsetof(ConfigData, checksum);
  for (size_t i = 0; i < checksumOffset; i++) {
    sum ^= data[i];
  }
  return sum;
}

bool ConfigStorage::verifyChecksum(const ConfigData& config) {
  return config.checksum == calculateChecksum(config);
}

bool ConfigStorage::hasValidConfig(const ConfigData& config) {
  return strlen(config.serverIP) > 0 && strlen(config.wifiSSID) > 0;
}
