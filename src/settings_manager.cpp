/*
 * Settings Manager Implementation
 */

#include "config.h"
#include "settings_manager.h"

SettingsManager::SettingsManager() {
  // Constructor
}

void SettingsManager::begin() {
  EEPROM.begin(512);  // Initialize EEPROM
  
  if (!load()) {
    DEBUG_PRINTLN(F("[SETTINGS] No valid settings, using defaults"));
    reset();
    save();
  } else {
    DEBUG_PRINTLN(F("[SETTINGS] Loaded successfully"));
    DEBUG_PRINT(F("[SETTINGS] Refresh: "));
    DEBUG_PRINT(settings.refreshInterval);
    DEBUG_PRINTLN(F("ms"));
  }
}

bool SettingsManager::load() {
  EEPROM.get(SETTINGS_EEPROM_OFFSET, settings);
  return isValid();
}

bool SettingsManager::save() {
  settings.magic = SETTINGS_MAGIC;  // Ensure magic is set
  EEPROM.put(SETTINGS_EEPROM_OFFSET, settings);
  bool success = EEPROM.commit();
  
  if (success) {
    DEBUG_PRINTLN(F("[SETTINGS] Saved successfully"));
  } else {
    DEBUG_PRINTLN(F("[SETTINGS] Save failed!"));
  }
  
  return success;
}

void SettingsManager::reset() {
  settings = UserSettings();  // Reset to defaults
  DEBUG_PRINTLN(F("[SETTINGS] Reset to defaults"));
}

void SettingsManager::setRefreshInterval(uint16_t interval) {
  // Validate interval (500ms to 60000ms)
  if (interval >= 500 && interval <= 60000) {
    settings.refreshInterval = interval;
    DEBUG_PRINT(F("[SETTINGS] Set refresh: "));
    DEBUG_PRINT(interval);
    DEBUG_PRINTLN(F("ms"));
  } else {
    DEBUG_PRINTLN(F("[SETTINGS] Invalid refresh interval!"));
  }
}

void SettingsManager::setDisplayMode(uint8_t mode) {
  settings.displayMode = mode;
}

const char* SettingsManager::getRefreshRateText() const {
  switch (settings.refreshInterval) {
    case 500:  return "0.5s (2Hz)";
    case 1000: return "1s (1Hz)";
    case 3000: return "3s (0.33Hz)";
    case 5000: return "5s (0.2Hz)";
    default:   return "Custom";
  }
}

void SettingsManager::cycleRefreshRate() {
  // Cycle: 5s -> 3s -> 1s -> 0.5s -> 5s
  switch (settings.refreshInterval) {
    case 5000:
      settings.refreshInterval = 3000;
      break;
    case 3000:
      settings.refreshInterval = 1000;
      break;
    case 1000:
      settings.refreshInterval = 500;
      break;
    case 500:
    default:
      settings.refreshInterval = 5000;
      break;
  }
  
  DEBUG_PRINT(F("[SETTINGS] Cycled to: "));
  DEBUG_PRINTLN(getRefreshRateText());
}
