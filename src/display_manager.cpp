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
  
  // Calculate responsive grid layout
  int margin = 2;  // Minimal margin for more space
  int tileSpacing = 2; // Minimal gap between tiles
  
  // Calculate tile dimensions based on screen size
  int tileWidth = (SCREEN_WIDTH - margin * 2 - tileSpacing) / 2; // 2 columns
  int tileHeight = (SCREEN_HEIGHT - 10 - margin * 2) / 4; // 4 rows (header + 4 rows)
  
  int x = margin;
  int y = margin;
  
  // Header bar at top
  tft->fillRect(0, 0, SCREEN_WIDTH, 10, COLOR_HEADER);
  tft->setTextSize(1);
  tft->setTextColor(COLOR_BG);
  drawCenteredText(1, "SYS", 1);
  y = 12;
  
  // Row 1: CPU and RAM tiles (side by side)
  // CPU Tile (left)
  tft->drawRect(x, y, tileWidth, tileHeight, COLOR_CPU);
  tft->setTextSize(1);
  tft->setTextColor(COLOR_CPU);
  tft->setCursor(x + 2, y + 2);
  tft->print(F("CPU"));
  
  // CPU percentage (large) - centered vertically
  tft->setTextSize(2);
  tft->setTextColor(COLOR_TEXT);
  int centerY = y + (tileHeight / 2) - 8;
  tft->setCursor(x + 4, centerY);
  tft->print((int)data.cpuLoad);
  tft->setTextSize(1);
  tft->print(F("%"));
  
  // CPU temp (bottom right corner)
  tft->setTextSize(1);
  tft->setTextColor(ST77XX_YELLOW);
  tft->setCursor(x + tileWidth - 24, y + tileHeight - 10);
  tft->print((int)data.cpuTemp);
  tft->print(F("C"));
  
  // CPU progress bar at bottom
  int barY = y + tileHeight - 4;
  drawProgressBar(x + 2, barY, tileWidth - 4, 2, data.cpuLoad, COLOR_CPU, COLOR_BG);
  
  // RAM Tile (right)
  int ramX = x + tileWidth + tileSpacing;
  tft->drawRect(ramX, y, tileWidth, tileHeight, COLOR_RAM);
  tft->setTextSize(1);
  tft->setTextColor(COLOR_RAM);
  tft->setCursor(ramX + 2, y + 2);
  tft->print(F("RAM"));
  
  float ramPercent = (data.ramTotal > 0) ? (data.ramUsed / data.ramTotal * 100.0) : 0;
  
  // RAM percentage (large) - centered vertically
  tft->setTextSize(2);
  tft->setTextColor(COLOR_TEXT);
  tft->setCursor(ramX + 4, centerY);
  tft->print((int)ramPercent);
  tft->setTextSize(1);
  tft->print(F("%"));
  
  // RAM usage (bottom right corner)
  tft->setTextSize(1);
  tft->setTextColor(ST77XX_CYAN);
  tft->setCursor(ramX + tileWidth - 28, y + tileHeight - 10);
  tft->print(data.ramUsed, 1);
  tft->print(F("G"));
  
  drawProgressBar(ramX + 2, barY, tileWidth - 4, 2, ramPercent, COLOR_RAM, COLOR_BG);
  
  y += tileHeight + tileSpacing;
  
  // Row 2: GPU and VRAM tiles (if available)
  if (data.gpuName.length() > 0) {
    centerY = y + (tileHeight / 2) - 8;
    
    // GPU Tile (left)
    tft->drawRect(x, y, tileWidth, tileHeight, COLOR_GPU);
    tft->setTextSize(1);
    tft->setTextColor(COLOR_GPU);
    tft->setCursor(x + 2, y + 2);
    tft->print(F("GPU"));
    
    // GPU percentage (large) - centered
    tft->setTextSize(2);
    tft->setTextColor(COLOR_TEXT);
    tft->setCursor(x + 4, centerY);
    tft->print((int)data.gpuLoad);
    tft->setTextSize(1);
    tft->print(F("%"));
    
    // GPU temp (bottom right)
    tft->setTextSize(1);
    tft->setTextColor(ST77XX_YELLOW);
    tft->setCursor(x + tileWidth - 24, y + tileHeight - 10);
    tft->print((int)data.gpuTemp);
    tft->print(F("C"));
    
    barY = y + tileHeight - 4;
    drawProgressBar(x + 2, barY, tileWidth - 4, 2, data.gpuLoad, COLOR_GPU, COLOR_BG);
    
    // VRAM Tile (right)
    if (data.gpuMemTotal > 0) {
      float vramPercent = (data.gpuMemUsed / (float)data.gpuMemTotal * 100.0);
      tft->drawRect(ramX, y, tileWidth, tileHeight, COLOR_VRAM);
      tft->setTextSize(1);
      tft->setTextColor(COLOR_VRAM);
      tft->setCursor(ramX + 2, y + 2);
      tft->print(F("VRAM"));
      
      // VRAM percentage (large) - centered
      tft->setTextSize(2);
      tft->setTextColor(COLOR_TEXT);
      tft->setCursor(ramX + 4, centerY);
      tft->print((int)vramPercent);
      tft->setTextSize(1);
      tft->print(F("%"));
      
      // VRAM usage (bottom right corner)
      tft->setTextSize(1);
      tft->setTextColor(ST77XX_CYAN);
      if (data.gpuMemUsed < 10000) {
        // Show in MB
        tft->setCursor(ramX + tileWidth - 32, y + tileHeight - 10);
        tft->print(data.gpuMemUsed);
        tft->print(F("M"));
      } else {
        // Show in GB
        tft->setCursor(ramX + tileWidth - 28, y + tileHeight - 10);
        tft->print(data.gpuMemUsed / 1024);
        tft->print(F("G"));
      }
      
      drawProgressBar(ramX + 2, barY, tileWidth - 4, 2, vramPercent, COLOR_VRAM, COLOR_BG);
    }
    
    y += tileHeight + tileSpacing;
  }
  
  // Row 3: Storage (full width)
  if (data.disk1Name.length() > 0 && y < SCREEN_HEIGHT - 30) {
    int storageWidth = SCREEN_WIDTH - margin * 2;
    tft->drawRect(x, y, storageWidth, tileHeight, COLOR_DISK);
    tft->setTextSize(1);
    tft->setTextColor(COLOR_DISK);
    tft->setCursor(x + 2, y + 2);
    tft->print(F("STORAGE"));
    
    // Calculate center for storage info
    int storageCenterY = y + (tileHeight / 2) - 4;
    
    // Show disk 1 info
    tft->setTextColor(COLOR_TEXT);
    tft->setCursor(x + 4, storageCenterY);
    tft->print(F("D1:"));
    tft->print((int)data.disk1Load);
    tft->print(F("%"));
    
    // Disk 1 temp on right side
    tft->setTextColor(ST77XX_YELLOW);
    tft->setCursor(x + storageWidth - 28, storageCenterY);
    tft->print((int)data.disk1Temp);
    tft->print(F("C"));
    
    // Show disk 2 if available (below disk 1)
    if (data.disk2Name.length() > 0) {
      tft->setTextColor(COLOR_TEXT);
      tft->setCursor(x + 4, storageCenterY + 10);
      tft->print(F("D2:"));
      tft->print((int)data.disk2Load);
      tft->print(F("%"));
      
      tft->setTextColor(ST77XX_YELLOW);
      tft->setCursor(x + storageWidth - 28, storageCenterY + 10);
      tft->print((int)data.disk2Temp);
      tft->print(F("C"));
    }
    
    barY = y + tileHeight - 4;
    // Use average of both disks if disk2 exists
    float avgLoad = data.disk2Name.length() > 0 ? 
                    (data.disk1Load + data.disk2Load) / 2.0 : data.disk1Load;
    drawProgressBar(x + 2, barY, storageWidth - 4, 2, avgLoad, COLOR_DISK, COLOR_BG);
    
    y += tileHeight + tileSpacing;
  }
  
  // Row 4: Network UP and DOWN (side by side)
  if (data.netName.length() > 0 && y < SCREEN_HEIGHT - 10) {
    centerY = y + (tileHeight / 2) - 8;
    
    // Upload Tile (left)
    tft->drawRect(x, y, tileWidth, tileHeight, COLOR_NET);
    tft->setTextSize(1);
    tft->setTextColor(COLOR_NET);
    tft->setCursor(x + 2, y + 2);
    tft->print(F("UP"));
    
    // Upload speed (centered)
    tft->setTextSize(2);
    tft->setTextColor(COLOR_TEXT);
    tft->setCursor(x + 4, centerY);
    if (data.netUp < 10) {
      tft->print(data.netUp, 1);
    } else if (data.netUp < 100) {
      tft->print((int)data.netUp);
    } else {
      tft->print((int)data.netUp);
    }
    
    // Unit (bottom left)
    tft->setTextSize(1);
    tft->setTextColor(ST77XX_GREEN);
    tft->setCursor(x + 2, y + tileHeight - 10);
    tft->print(F("Mb/s"));
    
    // Download Tile (right)
    tft->drawRect(ramX, y, tileWidth, tileHeight, COLOR_NET);
    tft->setTextSize(1);
    tft->setTextColor(COLOR_NET);
    tft->setCursor(ramX + 2, y + 2);
    tft->print(F("DOWN"));
    
    // Download speed (centered)
    tft->setTextSize(2);
    tft->setTextColor(COLOR_TEXT);
    tft->setCursor(ramX + 4, centerY);
    if (data.netDown < 10) {
      tft->print(data.netDown, 1);
    } else if (data.netDown < 100) {
      tft->print((int)data.netDown);
    } else {
      tft->print((int)data.netDown);
    }
    
    // Unit (bottom left)
    tft->setTextSize(1);
    tft->setTextColor(ST77XX_GREEN);
    tft->setCursor(ramX + 2, y + tileHeight - 10);
    tft->print(F("Mb/s"));
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

// Draw gaming-style progress bar with gradient effect
void DisplayManager::drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, float percent, uint16_t color, uint16_t bgColor) {
  if (percent > 100) percent = 100;
  if (percent < 0) percent = 0;
  
  // Draw border with accent
  tft->drawRect(x, y, w, h, color);
  
  // Fill background
  tft->fillRect(x + 1, y + 1, w - 2, h - 2, bgColor);
  
  // Calculate filled width
  int fillWidth = (int)((w - 2) * percent / 100.0);
  
  if (fillWidth > 0) {
    // Create gradient effect for gaming look
    for (int i = 0; i < fillWidth; i++) {
      // Brightness varies from 60% to 100% for depth effect
      float brightness = 0.6 + (0.4 * i / (float)fillWidth);
      
      uint16_t barColor = color;
      
      // Apply brightness (darken color at start)
      if (brightness < 1.0) {
        uint8_t r = ((color >> 11) & 0x1F) * brightness;
        uint8_t g = ((color >> 5) & 0x3F) * brightness;
        uint8_t b = (color & 0x1F) * brightness;
        barColor = (r << 11) | (g << 5) | b;
      }
      
      tft->drawFastVLine(x + 1 + i, y + 1, h - 2, barColor);
    }
    
    // Add highlight line at top for 3D effect
    if (h > 4) {
      tft->drawFastHLine(x + 1, y + 1, fillWidth, ST77XX_WHITE);
    }
  }
  
  // Warning colors at high usage
  if (percent > 90) {
    // Flash red border for critical
    tft->drawRect(x, y, w, h, ST77XX_RED);
  } else if (percent > 75) {
    // Yellow border for warning
    tft->drawRect(x, y, w, h, ST77XX_YELLOW);
  }
}

// Draw temperature gauge (circular indicator)
void DisplayManager::drawTemperatureGauge(int16_t x, int16_t y, int16_t size, float temp, float maxTemp, uint16_t color) {
  float percent = (temp / maxTemp) * 100.0;
  if (percent > 100) percent = 100;
  
  // Draw outer circle
  tft->drawCircle(x, y, size, color);
  
  // Fill based on temperature
  int fillRadius = (int)(size * percent / 100.0);
  tft->fillCircle(x, y, fillRadius, color);
  
  // Add warning colors
  if (percent > 80) {
    tft->drawCircle(x, y, size, ST77XX_RED);
  } else if (percent > 60) {
    tft->drawCircle(x, y, size, ST77XX_YELLOW);
  }
}

// Draw centered text (useful for headers)
void DisplayManager::drawCenteredText(int16_t y, const char* text, uint16_t color, uint8_t size) {
  tft->setTextSize(size);
  tft->setTextColor(color);
  
  // Calculate text width (approximate: 6 pixels per char * size)
  int textWidth = strlen(text) * 6 * size;
  int x = (SCREEN_WIDTH - textWidth) / 2;
  
  if (x < 0) x = 0;
  
  tft->setCursor(x, y);
  tft->print(text);
}
