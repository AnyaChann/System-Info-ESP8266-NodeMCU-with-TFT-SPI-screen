/*
 * Display Manager Implementation
 */

#include "config.h"  // MUST be first to define TFT_ST7735
#include "display_manager.h"
#include "version.h"
#include <ESP8266WiFi.h>  // For WiFi.localIP()

DisplayManager::DisplayManager(uint8_t cs, uint8_t dc, uint8_t rst, uint8_t led, uint8_t rot)
  : csPin(cs), dcPin(dc), rstPin(rst), ledPin(led), rotation(rot), displayOn(true) {
  
  #ifdef TFT_ST7735
    tft = new Adafruit_ST7735(csPin, dcPin, rstPin);
  #elif defined(TFT_ST7789)
    tft = new Adafruit_ST7789(csPin, dcPin, rstPin);
  #elif defined(TFT_ILI9341)
    tft = new Adafruit_ILI9341(csPin, dcPin, rstPin);
  #endif
}

DisplayManager::~DisplayManager() {
  delete tft;
}

void DisplayManager::begin() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  
  // Initialize display based on type
  #ifdef TFT_ST7735
    #ifdef ST7735_INITR
      tft->initR(ST7735_INITR);  // Use custom init code if defined
    #else
      tft->initR(INITR_BLACKTAB);  // Default to BLACKTAB
    #endif
  #elif defined(TFT_ST7789)
    tft->init(SCREEN_WIDTH, SCREEN_HEIGHT);
  #elif defined(TFT_ILI9341)
    tft->begin();
  #endif
  
  tft->setRotation(rotation);
  tft->fillScreen(COLOR_BG);
}

void DisplayManager::showSplashScreen() {
  tft->fillScreen(COLOR_BG);
  
  // Title
  tft->setTextColor(COLOR_HEADER);
  tft->setTextSize(2);
  tft->setCursor(10, 50);
  tft->println(F(PROJECT_NAME));
  
  // Version
  tft->setTextSize(1);
  tft->setTextColor(ST77XX_CYAN);
  tft->setCursor(35, 95);
  tft->print(F("v"));
  tft->println(F(PROJECT_VERSION));
  
  // Author
  tft->setTextColor(COLOR_TEXT);
  tft->setCursor(20, 120);
  tft->print(F("by "));
  tft->println(F(PROJECT_AUTHOR));
  
  delay(3000);
}

void DisplayManager::showWiFiConnecting() {
  tft->fillScreen(COLOR_BG);
  tft->setTextSize(2);
  tft->setTextColor(COLOR_HEADER);
  tft->setCursor(10, 50);
  tft->println(F("WiFi"));
  tft->setCursor(10, 70);
  tft->println(F("Connecting"));
  
  tft->setTextSize(1);
  tft->setTextColor(COLOR_TEXT);
  tft->setCursor(10, 100);
  tft->println(F("Please wait..."));
}

void DisplayManager::showWiFiStatus(bool success, String ip) {
  tft->fillScreen(COLOR_BG);
  tft->setTextSize(2);
  
  if (success) {
    tft->setTextColor(COLOR_HEADER);
    tft->setCursor(10, 40);
    tft->println(F("WiFi OK!"));
    
    tft->setTextSize(1);
    tft->setTextColor(COLOR_TEXT);
    tft->setCursor(5, 70);
    tft->println(F("Connected!"));
    
    // Show only last octet for privacy
    tft->setTextColor(ST77XX_CYAN);
    tft->setCursor(5, 85);
    int lastDot = ip.lastIndexOf('.');
    if (lastDot != -1) {
      tft->print(F("IP: ..."));
      tft->print(ip.substring(lastDot));
    } else {
      tft->print(F("IP: OK"));
    }
    
    // Reduced delay for faster boot
    delay(1000);
  } else {
    tft->setTextColor(COLOR_CPU);
    tft->setCursor(10, 40);
    tft->println(F("WiFi"));
    tft->setCursor(10, 60);
    tft->println(F("Failed!"));
    
    tft->setTextSize(1);
    tft->setTextColor(COLOR_TEXT);
    tft->setCursor(5, 90);
    tft->println(F("Will retry..."));
    
    // Reduced delay for faster reconnect
    delay(800);
  }
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
  
  // WiFi status (bottom) - only show last octet for privacy
  tft->setCursor(2, SCREEN_HEIGHT - 10);
  tft->setTextColor(COLOR_HEADER);
  tft->print(F("WiFi:"));
  
  // Extract last octet (e.g., "192.168.0.0" -> ".60")
  String ip = WiFi.localIP().toString();
  int lastDot = ip.lastIndexOf('.');
  if (lastDot != -1) {
    tft->print(F("..."));
    tft->print(ip.substring(lastDot));
  } else {
    tft->print(F("OK"));
  }
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

void DisplayManager::drawText(int16_t x, int16_t y, const char* text, uint16_t color, uint8_t size) {
  tft->setCursor(x, y);
  tft->setTextColor(color);
  tft->setTextSize(size);
  tft->println(text);
}

void DisplayManager::drawText(int16_t x, int16_t y, String text, uint16_t color, uint8_t size) {
  drawText(x, y, text.c_str(), color, size);
}
