/*
 * Settings Manager Module
 * Quản lý user settings (refresh rate, display preferences, etc)
 * Lưu vào EEPROM với offset khác ConfigManager
 */

#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>

// Settings storage offset (ConfigManager uses 0-118, we use 200+ to be safe)
#define SETTINGS_EEPROM_OFFSET 200
#define SETTINGS_MAGIC 0xFEED  // Magic number to verify settings

// User settings structure
struct UserSettings {
  uint16_t magic;              // Magic number for validation
  uint16_t refreshInterval;    // Refresh rate in milliseconds
  uint8_t displayMode;         // Display mode (0=Full, 1=Compact) - Future
  uint8_t reserved[10];        // Reserved for future use
  
  // Constructor with defaults
  UserSettings() : 
    magic(SETTINGS_MAGIC),
    refreshInterval(5000),     // Default 5s (0.2 Hz)
    displayMode(0) {
    memset(reserved, 0, sizeof(reserved));
  }
};

class SettingsManager {
private:
  UserSettings settings;
  
public:
  SettingsManager();
  
  // Initialize and load from EEPROM
  void begin();
  
  // Load/Save
  bool load();
  bool save();
  void reset();
  
  // Getters
  uint16_t getRefreshInterval() const { return settings.refreshInterval; }
  uint8_t getDisplayMode() const { return settings.displayMode; }
  
  // Setters
  void setRefreshInterval(uint16_t interval);
  void setDisplayMode(uint8_t mode);
  
  // Validation
  bool isValid() const { return settings.magic == SETTINGS_MAGIC; }
  
  // Refresh rate helpers
  const char* getRefreshRateText() const;
  void cycleRefreshRate();  // Cycle through available rates
};

#endif // SETTINGS_MANAGER_H
