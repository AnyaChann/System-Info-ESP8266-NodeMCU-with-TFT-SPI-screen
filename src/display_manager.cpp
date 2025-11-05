/*
 * Display Manager Implementation
 */

#include "config.h"  // MUST be first to define TFT_ST7735
#include "display_manager.h"
#include <ESP8266WiFi.h>  // For WiFi.localIP()

DisplayManager::DisplayManager(uint8_t cs, uint8_t dc, uint8_t rst, uint8_t led, uint8_t rot)
  : csPin(cs), dcPin(dc), rstPin(rst), ledPin(led), rotation(rot), displayOn(true) {
  
  #ifdef TFT_ST7735
    tft = new Adafruit_ST7735(csPin, dcPin, rstPin);
  #elif defined(TFT_ST7789)
    tft = new Adafruit_ST7789(csPin, dcPin, rstPin);
  #endif
}

DisplayManager::~DisplayManager() {
  delete tft;
}

void DisplayManager::begin() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  
  #ifdef TFT_ST7735
    tft->initR(INITR_BLACKTAB);
  #elif defined(TFT_ST7789)
    tft->init(SCREEN_WIDTH, SCREEN_HEIGHT);
  #endif
  
  tft->setRotation(rotation);
  tft->fillScreen(COLOR_BG);
}

void DisplayManager::showSplashScreen() {
  tft->fillScreen(COLOR_BG);
  tft->setTextColor(COLOR_HEADER);
  tft->setTextSize(2);
  tft->setCursor(10, 60);
  tft->println(F("System"));
  tft->setCursor(10, 80);
  tft->println(F("Monitor"));
}

void DisplayManager::showWiFiConnecting() {
  tft->setTextSize(1);
  tft->setCursor(10, 110);
  tft->setTextColor(COLOR_TEXT);
  tft->println(F("Connecting..."));
}

void DisplayManager::showWiFiStatus(bool success, String ip) {
  tft->fillRect(0, 110, SCREEN_WIDTH, 20, COLOR_BG);
  tft->setCursor(10, 110);
  tft->setTextColor(success ? COLOR_HEADER : COLOR_CPU);
  tft->println(success ? F("WiFi OK!") : F("WiFi Failed!"));
  delay(success ? 500 : 1000);
}

void DisplayManager::displaySystemInfo(const SystemData& data) {
  tft->fillScreen(COLOR_BG);
  
  int y = 2;
  
  // Header
  tft->setTextSize(1);
  tft->setTextColor(COLOR_HEADER);
  tft->setCursor(2, y);
  tft->print(F("SYSTEM MONITOR"));
  y += 12;
  
  // CPU
  tft->setTextColor(COLOR_CPU);
  tft->setCursor(2, y);
  tft->print(F("CPU:"));
  tft->setTextColor(COLOR_TEXT);
  tft->print((int)data.cpuTemp);
  tft->print(F("C "));
  tft->print((int)data.cpuLoad);
  tft->print(F("%"));
  y += 10;
  
  // RAM
  tft->setTextColor(COLOR_RAM);
  tft->setCursor(2, y);
  tft->print(F("RAM:"));
  tft->setTextColor(COLOR_TEXT);
  tft->print(data.ramUsed, 1);
  tft->print(F("/"));
  tft->print(data.ramTotal, 1);
  tft->print(F("GB"));
  y += 10;
  
  // GPU
  if (data.gpuName.length() > 0) {
    tft->setTextColor(COLOR_GPU);
    tft->setCursor(2, y);
    tft->print(F("GPU:"));
    tft->setTextColor(COLOR_TEXT);
    tft->print((int)data.gpuTemp);
    tft->print(F("C "));
    tft->print((int)data.gpuLoad);
    tft->print(F("%"));
    y += 10;
    
    if (data.gpuMemTotal > 0) {
      tft->setCursor(2, y);
      tft->print(F("VRAM:"));
      tft->print(data.gpuMemUsed);
      tft->print(F("/"));
      tft->print(data.gpuMemTotal);
      tft->print(F("MB"));
      y += 10;
    }
  }
  
  // Disk 1
  if (data.disk1Name.length() > 0) {
    tft->setTextColor(COLOR_DISK);
    tft->setCursor(2, y);
    tft->print(F("SSD1:"));
    tft->setTextColor(COLOR_TEXT);
    tft->print((int)data.disk1Temp);
    tft->print(F("C "));
    tft->print((int)data.disk1Load);
    tft->print(F("%"));
    y += 10;
  }
  
  // Disk 2
  if (data.disk2Name.length() > 0) {
    tft->setTextColor(COLOR_DISK);
    tft->setCursor(2, y);
    tft->print(F("SSD2:"));
    tft->setTextColor(COLOR_TEXT);
    tft->print((int)data.disk2Temp);
    tft->print(F("C "));
    tft->print((int)data.disk2Load);
    tft->print(F("%"));
    y += 10;
  }
  
  // Network
  if (data.netName.length() > 0) {
    tft->setTextColor(COLOR_NET);
    tft->setCursor(2, y);
    tft->print(F("NET:"));
    tft->setTextColor(COLOR_TEXT);
    tft->print(F("D:"));
    tft->print(data.netDown, 1);
    tft->print(F(" U:"));
    tft->print(data.netUp, 1);
    y += 10;
  }
  
  // WiFi status (bottom)
  tft->setCursor(2, SCREEN_HEIGHT - 10);
  tft->setTextColor(COLOR_HEADER);
  tft->print(F("WiFi:"));
  tft->print(WiFi.localIP().toString());
}

void DisplayManager::clear() {
  tft->fillScreen(COLOR_BG);
}

void DisplayManager::turnOn() {
  displayOn = true;
  digitalWrite(ledPin, HIGH);
  tft->fillScreen(COLOR_BG);
}

void DisplayManager::turnOff() {
  displayOn = false;
  digitalWrite(ledPin, LOW);
  tft->fillScreen(COLOR_BG);
}

void DisplayManager::toggle() {
  displayOn ? turnOff() : turnOn();
}

bool DisplayManager::isOn() {
  return displayOn;
}
