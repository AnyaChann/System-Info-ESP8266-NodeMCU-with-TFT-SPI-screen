/*
 * Menu Manager Module
 * State machine-based menu system with navigation
 */

#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H

#include <Arduino.h>

// Forward declarations
class DisplayManager;
class SettingsManager;
class ConfigManager;
class OTAWebManager;

// Menu states
enum MenuState {
  MENU_SYSTEM_INFO = 0,    // Back to dashboard
  MENU_REFRESH_RATE,       // Change refresh rate
  MENU_NETWORK_INFO,       // Show network info
  MENU_SERVER_CONFIG,      // Change server IP/Port
  MENU_WIFI_CONFIG,        // Change WiFi credentials
  MENU_OTA_UPDATE,         // OTA firmware update
  MENU_RESET_SERVER,       // Reset server config only
  MENU_RESET_WIFI,         // Reset WiFi config only
  MENU_FACTORY_RESET,      // Reset ALL config
  MENU_RESTART,            // Reboot ESP8266
  MENU_COUNT               // Total menu items
};

// Submenu states (for nested menus)
enum SubMenuState {
  SUBMENU_NONE = 0,
  SUBMENU_REFRESH_SELECT,     // Selecting refresh rate
  SUBMENU_NETWORK_DISPLAY,    // Showing network info
  SUBMENU_CONFIRM_RESET,      // Confirm factory reset (all)
  SUBMENU_CONFIRM_RESET_SERVER, // Confirm server reset
  SUBMENU_CONFIRM_RESET_WIFI, // Confirm WiFi reset
  SUBMENU_CONFIRM_RESTART     // Confirm restart
};

class MenuManager {
private:
  DisplayManager* display;
  SettingsManager* settings;
  ConfigManager* config;
  OTAWebManager* otaWeb;
  
  MenuState currentState;
  SubMenuState subMenuState;
  
  bool menuActive;
  unsigned long menuEnterTime;
  unsigned long lastInteractionTime;
  const unsigned long MENU_TIMEOUT = 10000;  // 10s auto-exit
  
  void (*onExitCallback)();  // Callback when menu exits
  
  // Menu rendering
  void drawMainMenu();
  void drawSubMenu();
  void drawMenuItem(int index, bool selected);
  const char* getMenuItemText(MenuState state);
  const char* getMenuItemIcon(MenuState state);
  
  // Submenu handlers
  void handleRefreshRateMenu();
  void handleNetworkInfoMenu();
  void handleConfirmDialog(const char* title, const char* message);
  
public:
  MenuManager(DisplayManager* disp, SettingsManager* sets, ConfigManager* cfg, OTAWebManager* ota);
  
  // Menu control
  void enter();                 // Enter menu (from System Info)
  void exit();                  // Exit to System Info
  bool isActive() const { return menuActive; }
  
  // Callback for exit notification
  void setExitCallback(void (*callback)()) { onExitCallback = callback; }
  
  // Navigation
  void next();                  // Next menu item
  void select();                // Select current item
  
  // Update (call in loop)
  void update();
  
  // Interaction tracking
  void resetTimeout();
  bool hasTimedOut() const;
};

#endif // MENU_MANAGER_H
