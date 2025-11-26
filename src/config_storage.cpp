/*
 * Config Storage Implementation
 */

#include "config.h"
#include "config_storage.h"
#include <string.h>

ConfigStorage::ConfigStorage() {
  EEPROM.begin(EEPROM_SIZE);
}

bool ConfigStorage::load(ConfigData& config) {
  EEPROM.get(0, config);
  
  DEBUG_PRINTLN(F("\n[STOR] Loading Config from EEPROM"));
  DEBUG_PRINTF("[STOR] Magic: 0x%X (expected: 0x%X)\n", config.magic, EEPROM_MAGIC);
  DEBUG_PRINT(F("[STOR] Version: "));
  DEBUG_PRINTLN(config.version);
  
  // Verify magic and version
  if (config.magic != EEPROM_MAGIC || config.version != EEPROM_VERSION) {
    DEBUG_PRINTLN(F("[STOR] Invalid magic or version"));
    return false;
  }
  
  // Verify checksum
  uint8_t expectedChecksum = calculateChecksum(config);
  DEBUG_PRINTF("[STOR] Checksum: 0x%X (expected: 0x%X)\n", config.checksum, expectedChecksum);
  
  if (!verifyChecksum(config)) {
    DEBUG_PRINTLN(F("[STOR] Checksum mismatch!"));
    return false;
  }
  
  // Check if required fields are filled (password can be empty - ESP WiFi stack saves it)
  DEBUG_PRINT(F("[STOR] Server IP: "));
  DEBUG_PRINTLN(config.serverIP);
  DEBUG_PRINT(F("[STOR] Server Port: "));
  DEBUG_PRINTLN(config.serverPort);
  DEBUG_PRINT(F("[STOR] WiFi SSID: "));
  DEBUG_PRINTLN(config.wifiSSID);
  DEBUG_PRINT(F("[STOR] WiFi Password: "));
  DEBUG_PRINTLN(strlen(config.wifiPassword) > 0 ? "***" : "(empty - using saved)");
  
  if (strlen(config.serverIP) == 0 || strlen(config.wifiSSID) == 0) {
    DEBUG_PRINTLN(F("[STOR] Required fields empty"));
    return false;
  }
  
  DEBUG_PRINTLN(F("[STOR] Config loaded successfully!"));
  return true;
}

bool ConfigStorage::save(const ConfigData& config) {
  ConfigData tempConfig = config;
  tempConfig.magic = EEPROM_MAGIC;
  tempConfig.version = EEPROM_VERSION;
  tempConfig.checksum = calculateChecksum(tempConfig);
  
  DEBUG_PRINTLN(F("\n[STOR] Saving Config to EEPROM"));
  DEBUG_PRINTF("[STOR] Magic: 0x%X\n", tempConfig.magic);
  DEBUG_PRINT(F("[STOR] Version: "));
  DEBUG_PRINTLN(tempConfig.version);
  DEBUG_PRINTF("[STOR] Server: %s:%d\n", tempConfig.serverIP, tempConfig.serverPort);
  DEBUG_PRINT(F("[STOR] WiFi: "));
  DEBUG_PRINT(tempConfig.wifiSSID);
  DEBUG_PRINTLN(F(" / ******"));
  DEBUG_PRINTF("[STOR] Checksum: 0x%X\n", tempConfig.checksum);
  
  EEPROM.put(0, tempConfig);
  bool success = EEPROM.commit();
  
  if (success) {
    DEBUG_PRINTLN(F("[STOR] Config saved to EEPROM"));
    
    // Verify by reading back
    ConfigData verify;
    EEPROM.get(0, verify);
    DEBUG_PRINTLN(F("[STOR] Verifying saved data"));
    DEBUG_PRINT(F("[STOR] Magic match: "));
    DEBUG_PRINTLN(verify.magic == EEPROM_MAGIC ? "YES" : "NO");
    DEBUG_PRINT(F("[STOR] Server IP match: "));
    DEBUG_PRINTLN(strcmp(verify.serverIP, tempConfig.serverIP) == 0 ? "YES" : "NO");
  } else {
    DEBUG_PRINTLN(F("[STOR] Failed to save config"));
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
