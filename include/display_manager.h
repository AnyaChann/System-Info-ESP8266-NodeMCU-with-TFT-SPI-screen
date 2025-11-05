/*
 * Display Manager Module
 * Quản lý TFT display và hiển thị thông tin
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "system_data.h"

// Include thư viện TFT phù hợp
#ifdef TFT_ST7735
  #include <Adafruit_ST7735.h>
  #define SCREEN_WIDTH  128
  #define SCREEN_HEIGHT 160
#elif defined(TFT_ST7789)
  #include <Adafruit_ST7789.h>
  #define SCREEN_WIDTH  135
  #define SCREEN_HEIGHT 240
#else
  #error "Please define TFT_ST7735 or TFT_ST7789 in config.h"
#endif

class DisplayManager {
private:
  #ifdef TFT_ST7735
    Adafruit_ST7735* tft;
  #elif defined(TFT_ST7789)
    Adafruit_ST7789* tft;
  #endif
  
  uint8_t csPin, dcPin, rstPin, ledPin;
  uint8_t rotation;
  bool displayOn;
  
public:
  DisplayManager(uint8_t cs, uint8_t dc, uint8_t rst, uint8_t led, uint8_t rot = 1);
  ~DisplayManager();
  
  void begin();
  void showSplashScreen();
  void showWiFiConnecting();
  void showWiFiStatus(bool success, String ip = "");
  void displaySystemInfo(const SystemData& data);
  void clear();
  void turnOn();
  void turnOff();
  void toggle();
  bool isOn();
  
  // Helper methods for config portal
  void drawText(int16_t x, int16_t y, const char* text, uint16_t color, uint8_t size = 1);
  void drawText(int16_t x, int16_t y, String text, uint16_t color, uint8_t size = 1);
};

#endif // DISPLAY_MANAGER_H
