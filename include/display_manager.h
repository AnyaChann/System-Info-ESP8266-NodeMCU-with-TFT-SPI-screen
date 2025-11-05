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
  #ifndef TFT_WIDTH
    #define TFT_WIDTH  128
  #endif
  #ifndef TFT_HEIGHT
    #define TFT_HEIGHT 160
  #endif
  #define SCREEN_WIDTH  TFT_WIDTH
  #define SCREEN_HEIGHT TFT_HEIGHT
  
#elif defined(TFT_ST7789)
  #include <Adafruit_ST7789.h>
  #ifndef TFT_WIDTH
    #define TFT_WIDTH  240
  #endif
  #ifndef TFT_HEIGHT
    #define TFT_HEIGHT 240
  #endif
  #define SCREEN_WIDTH  TFT_WIDTH
  #define SCREEN_HEIGHT TFT_HEIGHT
  
#elif defined(TFT_ILI9341)
  #include <Adafruit_ILI9341.h>
  #ifndef TFT_WIDTH
    #define TFT_WIDTH  240
  #endif
  #ifndef TFT_HEIGHT
    #define TFT_HEIGHT 320
  #endif
  #define SCREEN_WIDTH  TFT_WIDTH
  #define SCREEN_HEIGHT TFT_HEIGHT
  
#else
  #error "Please define a display type (TFT_ST7735, TFT_ST7789, or TFT_ILI9341) in config.h"
#endif

class DisplayManager {
private:
  // Polymorphic pointer based on display type
  #ifdef TFT_ST7735
    Adafruit_ST7735* tft;
  #elif defined(TFT_ST7789)
    Adafruit_ST7789* tft;
  #elif defined(TFT_ILI9341)
    Adafruit_ILI9341* tft;
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
