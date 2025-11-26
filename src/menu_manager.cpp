/*
 * Menu Manager Implementation
 */

#include "config.h"
#include "menu_manager.h"
#include "display_manager.h"
#include "settings_manager.h"
#include "config_manager.h"
#include "ota_web_manager.h"

MenuManager::MenuManager(DisplayManager* disp, SettingsManager* sets, ConfigManager* cfg, OTAWebManager* ota)
  : display(disp), settings(sets), config(cfg), otaWeb(ota),
    currentState(MENU_SYSTEM_INFO), subMenuState(SUBMENU_NONE),
    menuActive(false), menuEnterTime(0), lastInteractionTime(0), onExitCallback(nullptr) {}

void MenuManager::enter() {
  menuActive = true;
  currentState = MENU_SYSTEM_INFO;
  subMenuState = SUBMENU_NONE;
  menuEnterTime = millis();
  lastInteractionTime = millis();
  
  DEBUG_PRINTLN(F("[MENU] Entered"));
  drawMainMenu();
}

void MenuManager::exit() {
  menuActive = false;
  subMenuState = SUBMENU_NONE;
  
  DEBUG_PRINTLN(F("[MENU] Exited"));
  
  // Clear display - main loop will redraw system info
  if (display) {
    display->clear();
  }
  
  // Notify main loop to force refresh
  if (onExitCallback) {
    onExitCallback();
  }
}

void MenuManager::next() {
  if (!menuActive) return;
  
  resetTimeout();
  
  // Handle submenu navigation
  if (subMenuState != SUBMENU_NONE) {
    // Submenu specific navigation handled in select()
    return;
  }
  
  // Main menu navigation
  currentState = (MenuState)((currentState + 1) % MENU_COUNT);
  
  DEBUG_PRINT(F("[MENU] Navigate to: "));
  DEBUG_PRINTLN(getMenuItemText(currentState));
  
  drawMainMenu();
}

void MenuManager::select() {
  if (!menuActive) return;
  
  resetTimeout();
  
  // Handle submenu selection
  if (subMenuState != SUBMENU_NONE) {
    switch (subMenuState) {
      case SUBMENU_REFRESH_SELECT:
        // Cycle refresh rate
        settings->cycleRefreshRate();
        settings->save();
        handleRefreshRateMenu();
        break;
        
      case SUBMENU_NETWORK_DISPLAY:
        // Exit network info
        subMenuState = SUBMENU_NONE;
        drawMainMenu();
        break;
        
      case SUBMENU_CONFIRM_RESET:
        // Factory reset confirmed (ALL)
        DEBUG_PRINTLN(F("[MENU] Factory reset confirmed!"));
        if (display) {
          display->clear();
          display->drawText(15, 50, "RESET ALL", ST77XX_RED, 2);
          display->drawText(20, 80, "Please wait", ST77XX_WHITE, 1);
        }
        delay(2000);
        config->resetConfig();
        ESP.restart();
        break;
        
      case SUBMENU_CONFIRM_RESET_SERVER:
        // Reset server only
        DEBUG_PRINTLN(F("[MENU] Reset server confirmed!"));
        if (display) {
          display->clear();
          display->drawText(10, 50, "RESET SERVER", ST77XX_YELLOW, 2);
          display->drawText(20, 80, "WiFi kept!", ST77XX_GREEN, 1);
        }
        delay(2000);
        config->resetServerConfig();
        ESP.restart();
        break;
        
      case SUBMENU_CONFIRM_RESET_WIFI:
        // Reset WiFi only
        DEBUG_PRINTLN(F("[MENU] Reset WiFi confirmed!"));
        if (display) {
          display->clear();
          display->drawText(10, 50, "RESET WIFI", ST77XX_YELLOW, 2);
          display->drawText(15, 80, "Server kept!", ST77XX_GREEN, 1);
        }
        delay(2000);
        config->resetWiFiConfig();
        ESP.restart();
        break;
        
      case SUBMENU_CONFIRM_RESTART:
        // Restart confirmed
        DEBUG_PRINTLN(F("[MENU] Restart confirmed!"));
        if (display) {
          display->clear();
          display->drawText(20, 50, "RESTARTING", ST77XX_YELLOW, 2);
        }
        delay(1500);
        ESP.restart();
        break;
        
      default:
        break;
    }
    return;
  }
  
  // Main menu selection
  DEBUG_PRINT(F("[MENU] Selected: "));
  DEBUG_PRINTLN(getMenuItemText(currentState));
  
  switch (currentState) {
    case MENU_SYSTEM_INFO:
      // Exit to system info
      exit();
      break;
      
    case MENU_REFRESH_RATE:
      // Enter refresh rate submenu
      subMenuState = SUBMENU_REFRESH_SELECT;
      handleRefreshRateMenu();
      break;
      
    case MENU_NETWORK_INFO:
      // Show network info
      subMenuState = SUBMENU_NETWORK_DISPLAY;
      handleNetworkInfoMenu();
      break;
      
    case MENU_SERVER_CONFIG:
      // Start server config portal
      DEBUG_PRINTLN(F("[MENU] Starting server config..."));
      exit();
      config->startConfigPortalWithValidation();
      break;
      
    case MENU_WIFI_CONFIG:
      // Start WiFi config portal
      DEBUG_PRINTLN(F("[MENU] Starting WiFi config..."));
      exit();
      config->startConfigPortalWithValidation();
      break;
      
    case MENU_OTA_UPDATE:
      // Start OTA web interface
      DEBUG_PRINTLN(F("[MENU] Starting OTA mode..."));
      exit();
      if (otaWeb && !otaWeb->active()) {
        otaWeb->start(WiFi.localIP().toString());
      }
      break;
      
    case MENU_RESET_SERVER:
      // Confirm server reset
      subMenuState = SUBMENU_CONFIRM_RESET_SERVER;
      handleConfirmDialog("RESET SERVER", "WiFi kept!");
      break;
      
    case MENU_RESET_WIFI:
      // Confirm WiFi reset
      subMenuState = SUBMENU_CONFIRM_RESET_WIFI;
      handleConfirmDialog("RESET WIFI", "Server kept!");
      break;
      
    case MENU_FACTORY_RESET:
      // Confirm factory reset (ALL)
      subMenuState = SUBMENU_CONFIRM_RESET;
      handleConfirmDialog("RESET ALL", "WiFi + Server!");
      break;
      
    case MENU_RESTART:
      // Confirm restart
      subMenuState = SUBMENU_CONFIRM_RESTART;
      handleConfirmDialog("RESTART", "Hold to confirm");
      break;
      
    default:
      break;
  }
}

void MenuManager::update() {
  if (!menuActive) return;
  
  // Check timeout
  if (hasTimedOut()) {
    DEBUG_PRINTLN(F("[MENU] Timeout - auto exit"));
    exit();
  }
}

void MenuManager::resetTimeout() {
  lastInteractionTime = millis();
}

bool MenuManager::hasTimedOut() const {
  return (millis() - lastInteractionTime) > MENU_TIMEOUT;
}

// ============= Rendering =============

void MenuManager::drawMainMenu() {
  if (!display) return;
  
  display->clear();
  
  // Title
  display->drawText(30, 5, "MENU", ST77XX_CYAN, 2);
  
  // Draw 3 items: current, prev, next (scrolling effect)
  int startY = 35;
  int itemHeight = 30;
  
  for (int i = -1; i <= 1; i++) {
    int index = (currentState + i + MENU_COUNT) % MENU_COUNT;
    bool selected = (i == 0);
    
    int y = startY + (i + 1) * itemHeight;
    
    if (selected) {
      // Highlight selected
      display->drawText(5, y, ">", ST77XX_YELLOW, 2);
      display->drawText(20, y, getMenuItemIcon((MenuState)index), ST77XX_YELLOW, 1);
      display->drawText(35, y, getMenuItemText((MenuState)index), ST77XX_WHITE, 1);
    } else {
      // Non-selected (dimmed)
      display->drawText(20, y, getMenuItemIcon((MenuState)index), 0x7BEF, 1);
      display->drawText(35, y, getMenuItemText((MenuState)index), 0x7BEF, 1);
    }
  }
  
  // Footer hint
  display->drawText(10, 125, "Press: Next", ST77XX_CYAN, 1);
  display->drawText(10, 135, "Hold: Select", ST77XX_CYAN, 1);
}

void MenuManager::handleRefreshRateMenu() {
  if (!display || !settings) return;
  
  display->clear();
  display->drawText(15, 10, "REFRESH RATE", ST77XX_CYAN, 1);
  
  // Show current rate
  display->drawText(20, 40, "Current:", ST77XX_WHITE, 1);
  display->drawText(20, 60, settings->getRefreshRateText(), ST77XX_YELLOW, 2);
  
  // Instructions
  display->drawText(10, 100, "Press: Change", ST77XX_GREEN, 1);
  display->drawText(10, 115, "Wait: Back", ST77XX_WHITE, 1);
}

void MenuManager::handleNetworkInfoMenu() {
  if (!display) return;
  
  display->clear();
  display->drawText(15, 5, "NETWORK INFO", ST77XX_CYAN, 1);
  
  // Show network info
  display->drawText(5, 25, "SSID:", ST77XX_WHITE, 1);
  display->drawText(5, 40, WiFi.SSID().c_str(), ST77XX_YELLOW, 1);
  
  display->drawText(5, 60, "IP:", ST77XX_WHITE, 1);
  display->drawText(5, 75, WiFi.localIP().toString().c_str(), ST77XX_GREEN, 1);
  
  display->drawText(5, 95, "Signal:", ST77XX_WHITE, 1);
  char rssi[16];
  snprintf(rssi, sizeof(rssi), "%d dBm", WiFi.RSSI());
  display->drawText(5, 110, rssi, ST77XX_CYAN, 1);
  
  // Uptime
  display->drawText(5, 130, "Uptime:", ST77XX_WHITE, 1);
  unsigned long uptime = millis() / 1000;
  char uptimeStr[16];
  snprintf(uptimeStr, sizeof(uptimeStr), "%lum %lus", uptime / 60, uptime % 60);
  display->drawText(5, 145, uptimeStr, ST77XX_WHITE, 1);
}

void MenuManager::handleConfirmDialog(const char* title, const char* message) {
  if (!display) return;
  
  display->clear();
  
  // Warning
  display->drawText(30, 40, title, ST77XX_RED, 1);
  display->drawText(20, 70, message, ST77XX_YELLOW, 1);
  
  // Instructions
  display->drawText(15, 110, "Hold: Confirm", ST77XX_WHITE, 1);
  display->drawText(15, 125, "Wait: Cancel", ST77XX_CYAN, 1);
}

const char* MenuManager::getMenuItemText(MenuState state) {
  switch (state) {
    case MENU_SYSTEM_INFO:    return "System Info";
    case MENU_REFRESH_RATE:   return "Refresh Rate";
    case MENU_NETWORK_INFO:   return "Network Info";
    case MENU_SERVER_CONFIG:  return "Server Config";
    case MENU_WIFI_CONFIG:    return "WiFi Config";
    case MENU_OTA_UPDATE:     return "OTA Update";
    case MENU_RESET_SERVER:   return "Reset Server";
    case MENU_RESET_WIFI:     return "Reset WiFi";
    case MENU_FACTORY_RESET:  return "Reset All";
    case MENU_RESTART:        return "Restart";
    default:                  return "Unknown";
  }
}

const char* MenuManager::getMenuItemIcon(MenuState state) {
  switch (state) {
    case MENU_SYSTEM_INFO:    return "*";
    case MENU_REFRESH_RATE:   return "o";
    case MENU_NETWORK_INFO:   return "~";
    case MENU_SERVER_CONFIG:  return "#";
    case MENU_WIFI_CONFIG:    return "@";
    case MENU_OTA_UPDATE:     return "^";
    case MENU_RESET_SERVER:   return "x";
    case MENU_RESET_WIFI:     return "X";
    case MENU_FACTORY_RESET:  return "!";
    case MENU_RESTART:        return "+";
    default:                  return "?";
  }
}
