/*
 * Config Storage Module - EEPROM operations
 * Handles saving/loading configuration data
 */

#ifndef CONFIG_STORAGE_H
#define CONFIG_STORAGE_H

#include <Arduino.h>
#include <EEPROM.h>

// EEPROM Layout
#define EEPROM_SIZE 512
#define EEPROM_MAGIC 0x4553  // "ES" magic number
#define EEPROM_VERSION 1

// Config structure
struct ConfigData {
  uint16_t magic;           // Magic number để verify
  uint8_t version;          // Config version
  
  // Server config
  char serverIP[16];        // "192.168.2.60"
  uint16_t serverPort;      // 8080
  
  // WiFi config
  char wifiSSID[32];        // WiFi name
  char wifiPassword[64];    // WiFi password
  
  uint8_t checksum;         // Simple checksum
};

class ConfigStorage {
public:
  ConfigStorage();
  
  // EEPROM operations
  bool load(ConfigData& config);
  bool save(const ConfigData& config);
  void clear(ConfigData& config);
  
  // Validation
  uint8_t calculateChecksum(const ConfigData& config);
  bool verifyChecksum(const ConfigData& config);
  bool hasValidConfig(const ConfigData& config);

private:
  // No state needed
};

#endif // CONFIG_STORAGE_H
