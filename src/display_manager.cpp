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
  Serial.println("Setting up LED backlight...");
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  Serial.println("LED backlight ON");
  
  Serial.println("Initializing TFT display...");
  
  #ifdef TFT_ST7735
    tft->initR(INITR_BLACKTAB);
  #elif defined(TFT_ST7789)
    tft->init(SCREEN_WIDTH, SCREEN_HEIGHT);
  #endif
  
  tft->setRotation(rotation);
  tft->fillScreen(COLOR_BG);
  Serial.println("TFT display initialized!");
}

void DisplayManager::showSplashScreen() {
  Serial.println("Drawing splash screen...");
  
  // Test màu
  tft->fillScreen(ST77XX_RED);
  delay(500);
  tft->fillScreen(ST77XX_GREEN);
  delay(500);
  tft->fillScreen(ST77XX_BLUE);
  delay(500);
  
  // Splash screen
  tft->fillScreen(COLOR_BG);
  tft->setTextColor(COLOR_HEADER);
  tft->setTextSize(2);
  tft->setCursor(10, 60);
  tft->println("System");
  tft->setCursor(10, 80);
  tft->println("Monitor");
  
  Serial.println("Splash screen drawn!");
}

void DisplayManager::showWiFiConnecting() {
  tft->setTextSize(1);
  tft->setCursor(10, 110);
  tft->setTextColor(COLOR_TEXT);
  tft->println("Connecting WiFi...");
}

void DisplayManager::showWiFiStatus(bool success, String ip) {
  tft->fillRect(0, 110, SCREEN_WIDTH, 20, COLOR_BG);
  tft->setCursor(10, 110);
  
  if (success) {
    tft->setTextColor(COLOR_HEADER);
    tft->println("WiFi OK!");
    delay(1000);
  } else {
    tft->setTextColor(COLOR_CPU);
    tft->println("WiFi Failed!");
    delay(2000);
  }
}

void DisplayManager::displaySystemInfo(const SystemData& data) {
  tft->fillScreen(COLOR_BG);
  
  int y = 2;
  
  // Header
  tft->setTextSize(1);
  tft->setTextColor(COLOR_HEADER);
  tft->setCursor(2, y);
  tft->print("SYSTEM MONITOR");
  y += 12;
  
  // CPU
  tft->setTextColor(COLOR_CPU);
  tft->setCursor(2, y);
  tft->print("CPU:");
  tft->setTextColor(COLOR_TEXT);
  tft->print(String(data.cpuTemp, 0) + "C ");
  tft->print(String(data.cpuLoad, 0) + "%");
  y += 10;
  
  // RAM
  tft->setTextColor(COLOR_RAM);
  tft->setCursor(2, y);
  tft->print("RAM:");
  tft->setTextColor(COLOR_TEXT);
  tft->print(String(data.ramUsed, 1) + "/" + String(data.ramTotal, 1) + "GB");
  y += 10;
  
  // GPU
  if (data.gpuName.length() > 0) {
    tft->setTextColor(COLOR_GPU);
    tft->setCursor(2, y);
    tft->print("GPU:");
    tft->setTextColor(COLOR_TEXT);
    tft->print(String(data.gpuTemp, 0) + "C ");
    tft->print(String(data.gpuLoad, 0) + "%");
    y += 10;
    
    if (data.gpuMemTotal > 0) {
      tft->setCursor(2, y);
      tft->print("VRAM:" + String(data.gpuMemUsed) + "/" + String(data.gpuMemTotal) + "MB");
      y += 10;
    }
  }
  
  // Disk 1
  if (data.disk1Name.length() > 0) {
    tft->setTextColor(COLOR_DISK);
    tft->setCursor(2, y);
    tft->print("SSD1:");
    tft->setTextColor(COLOR_TEXT);
    tft->print(String(data.disk1Temp, 0) + "C ");
    tft->print(String(data.disk1Load, 0) + "%");
    y += 10;
  }
  
  // Disk 2
  if (data.disk2Name.length() > 0) {
    tft->setTextColor(COLOR_DISK);
    tft->setCursor(2, y);
    tft->print("SSD2:");
    tft->setTextColor(COLOR_TEXT);
    tft->print(String(data.disk2Temp, 0) + "C ");
    tft->print(String(data.disk2Load, 0) + "%");
    y += 10;
  }
  
  // Network
  if (data.netName.length() > 0) {
    tft->setTextColor(COLOR_NET);
    tft->setCursor(2, y);
    tft->print("NET:");
    tft->setTextColor(COLOR_TEXT);
    tft->print("D:" + String(data.netDown, 1) + " U:" + String(data.netUp, 1));
    y += 10;
  }
  
  // WiFi status (bottom)
  tft->setCursor(2, SCREEN_HEIGHT - 10);
  tft->setTextColor(COLOR_HEADER);
  tft->print("WiFi: ");
  tft->print(WiFi.localIP().toString());
}

void DisplayManager::clear() {
  tft->fillScreen(COLOR_BG);
}

void DisplayManager::turnOn() {
  displayOn = true;
  digitalWrite(ledPin, HIGH);
  Serial.println("Display ON");
  tft->fillScreen(COLOR_BG);
}

void DisplayManager::turnOff() {
  displayOn = false;
  digitalWrite(ledPin, LOW);
  Serial.println("Display OFF");
  tft->fillScreen(COLOR_BG);
}

void DisplayManager::toggle() {
  Serial.print("toggleDisplay() called. New state: ");
  Serial.println(!displayOn ? "ON" : "OFF");
  
  if (displayOn) {
    turnOff();
  } else {
    turnOn();
  }
}

bool DisplayManager::isOn() {
  return displayOn;
}
