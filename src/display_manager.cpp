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
  
  // Get actual display dimensions after rotation is applied
  int16_t screenWidth = tft->width();
  int16_t screenHeight = tft->height();
  
  // Auto-detect orientation based on actual screen dimensions after rotation
  // Portrait: height > width (rotation 0 or 2)
  // Landscape: width > height (rotation 1 or 3)
  bool isLandscape = (screenWidth > screenHeight);
  
  int tileWidth, tileHeight, numCols, numRows;
  
  if (isLandscape) {
    // Landscape mode: 3 columns x 2-3 rows
    numCols = 3;
    numRows = 2;
    tileWidth = (screenWidth - margin * 2 - tileSpacing * (numCols - 1)) / numCols;
    tileHeight = (screenHeight - 10 - margin * 2 - tileSpacing) / numRows;
  } else {
    // Portrait mode: 2 columns x 4 rows
    numCols = 2;
    numRows = 4;
    tileWidth = (screenWidth - margin * 2 - tileSpacing) / numCols;
    tileHeight = (screenHeight - 10 - margin * 2) / numRows;
  }
  
  int x = margin;
  int y = margin;
  
  // Header bar at top
  tft->fillRect(0, 0, screenWidth, 10, COLOR_HEADER);
  tft->setTextSize(1);
  tft->setTextColor(COLOR_BG);
  drawCenteredText(1, "SYS", 1);
  y = 12;
  
  // Draw tiles based on orientation
  if (isLandscape) {
    // === LANDSCAPE MODE: 3 columns ===
    // Row 1: CPU | RAM | GPU
    // Row 2: VRAM | STORAGE | NET
    
    // CPU Tile (col 1)
    drawTile_CPU(x, y, tileWidth, tileHeight, data);
    
    // RAM Tile (col 2)
    int col2X = x + tileWidth + tileSpacing;
    drawTile_RAM(col2X, y, tileWidth, tileHeight, data);
    
    // GPU Tile (col 3)
    if (data.gpuName.length() > 0) {
      int col3X = col2X + tileWidth + tileSpacing;
      drawTile_GPU(col3X, y, tileWidth, tileHeight, data);
    }
    
    y += tileHeight + tileSpacing;
    
    // VRAM Tile (col 1)
    if (data.gpuName.length() > 0 && data.gpuMemTotal > 0) {
      drawTile_VRAM(x, y, tileWidth, tileHeight, data);
    }
    
    // Storage Tile (col 2)
    if (data.disk1Name.length() > 0) {
      drawTile_Storage(col2X, y, tileWidth, tileHeight, data);
    }
    
    // Network Tile (col 3) - combined UP+DOWN
    if (data.netName.length() > 0) {
      int col3X = col2X + tileWidth + tileSpacing;
      drawTile_Network_Combined(col3X, y, tileWidth, tileHeight, data);
    }
    
  } else {
    // === PORTRAIT MODE: 2 columns ===
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
    }
    
    y += tileHeight + tileSpacing;
  }
  
  // Row 3: Storage (full width)
  if (data.disk1Name.length() > 0 && y < screenHeight - 30) {
    int storageWidth = screenWidth - margin * 2;
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
  } // End of portrait/landscape conditional
}

// Helper function to draw CPU tile
void DisplayManager::drawTile_CPU(int x, int y, int w, int h, const SystemData& data) {
  tft->drawRect(x, y, w, h, COLOR_CPU);
  tft->setTextSize(1);
  tft->setTextColor(COLOR_CPU);
  tft->setCursor(x + 2, y + 2);
  tft->print(F("CPU"));
  
  int centerY = y + (h / 2) - 8;
  tft->setTextSize(2);
  tft->setTextColor(COLOR_TEXT);
  tft->setCursor(x + 4, centerY);
  tft->print((int)data.cpuLoad);
  tft->setTextSize(1);
  tft->print(F("%"));
  
  tft->setTextSize(1);
  tft->setTextColor(ST77XX_YELLOW);
  tft->setCursor(x + w - 24, y + h - 10);
  tft->print((int)data.cpuTemp);
  tft->print(F("C"));
}

// Helper function to draw RAM tile
void DisplayManager::drawTile_RAM(int x, int y, int w, int h, const SystemData& data) {
  tft->drawRect(x, y, w, h, COLOR_RAM);
  tft->setTextSize(1);
  tft->setTextColor(COLOR_RAM);
  tft->setCursor(x + 2, y + 2);
  tft->print(F("RAM"));
  
  float ramPercent = (data.ramTotal > 0) ? (data.ramUsed / data.ramTotal * 100.0) : 0;
  int centerY = y + (h / 2) - 8;
  tft->setTextSize(2);
  tft->setTextColor(COLOR_TEXT);
  tft->setCursor(x + 4, centerY);
  tft->print((int)ramPercent);
  tft->setTextSize(1);
  tft->print(F("%"));
  
  tft->setTextSize(1);
  tft->setTextColor(ST77XX_CYAN);
  tft->setCursor(x + w - 28, y + h - 10);
  tft->print(data.ramUsed, 1);
  tft->print(F("G"));
}

// Helper function to draw GPU tile
void DisplayManager::drawTile_GPU(int x, int y, int w, int h, const SystemData& data) {
  tft->drawRect(x, y, w, h, COLOR_GPU);
  tft->setTextSize(1);
  tft->setTextColor(COLOR_GPU);
  tft->setCursor(x + 2, y + 2);
  tft->print(F("GPU"));
  
  int centerY = y + (h / 2) - 8;
  tft->setTextSize(2);
  tft->setTextColor(COLOR_TEXT);
  tft->setCursor(x + 4, centerY);
  tft->print((int)data.gpuLoad);
  tft->setTextSize(1);
  tft->print(F("%"));
  
  tft->setTextSize(1);
  tft->setTextColor(ST77XX_YELLOW);
  tft->setCursor(x + w - 24, y + h - 10);
  tft->print((int)data.gpuTemp);
  tft->print(F("C"));
}

// Helper function to draw VRAM tile
void DisplayManager::drawTile_VRAM(int x, int y, int w, int h, const SystemData& data) {
  float vramPercent = (data.gpuMemUsed / (float)data.gpuMemTotal * 100.0);
  tft->drawRect(x, y, w, h, COLOR_VRAM);
  tft->setTextSize(1);
  tft->setTextColor(COLOR_VRAM);
  tft->setCursor(x + 2, y + 2);
  tft->print(F("VRAM"));
  
  int centerY = y + (h / 2) - 8;
  tft->setTextSize(2);
  tft->setTextColor(COLOR_TEXT);
  tft->setCursor(x + 4, centerY);
  tft->print((int)vramPercent);
  tft->setTextSize(1);
  tft->print(F("%"));
  
  tft->setTextSize(1);
  tft->setTextColor(ST77XX_CYAN);
  if (data.gpuMemUsed < 10000) {
    tft->setCursor(x + w - 32, y + h - 10);
    tft->print(data.gpuMemUsed);
    tft->print(F("M"));
  } else {
    tft->setCursor(x + w - 28, y + h - 10);
    tft->print(data.gpuMemUsed / 1024);
    tft->print(F("G"));
  }
}

// Helper function to draw Storage tile
void DisplayManager::drawTile_Storage(int x, int y, int w, int h, const SystemData& data) {
  tft->drawRect(x, y, w, h, COLOR_DISK);
  tft->setTextSize(1);
  tft->setTextColor(COLOR_DISK);
  tft->setCursor(x + 2, y + 2);
  tft->print(F("SSD"));
  
  // For narrow tiles (landscape), use vertical layout
  // For wide tiles (portrait full-width), use horizontal layout
  bool narrowTile = (w < 100);
  
  if (narrowTile) {
    // Vertical layout for narrow tiles (landscape mode)
    int lineY = y + 12;
    
    // D1 load
    tft->setTextColor(COLOR_TEXT);
    tft->setCursor(x + 2, lineY);
    tft->print(F("D1:"));
    tft->print((int)data.disk1Load);
    tft->print(F("%"));
    
    // D1 temp below
    tft->setTextColor(ST77XX_YELLOW);
    tft->setCursor(x + 2, lineY + 10);
    tft->print((int)data.disk1Temp);
    tft->print(F("C"));
    
    if (data.disk2Name.length() > 0) {
      lineY += 20;
      // D2 load
      tft->setTextColor(COLOR_TEXT);
      tft->setCursor(x + 2, lineY);
      tft->print(F("D2:"));
      tft->print((int)data.disk2Load);
      tft->print(F("%"));
      
      // D2 temp below
      tft->setTextColor(ST77XX_YELLOW);
      tft->setCursor(x + 2, lineY + 10);
      tft->print((int)data.disk2Temp);
      tft->print(F("C"));
    }
  } else {
    // Horizontal layout for wide tiles (portrait mode)
    int centerY = y + (h / 2) - 4;
    
    tft->setTextColor(COLOR_TEXT);
    tft->setCursor(x + 4, centerY);
    tft->print(F("D1:"));
    tft->print((int)data.disk1Load);
    tft->print(F("%"));
    
    tft->setTextColor(ST77XX_YELLOW);
    tft->setCursor(x + w - 28, centerY);
    tft->print((int)data.disk1Temp);
    tft->print(F("C"));
    
    if (data.disk2Name.length() > 0) {
      tft->setTextColor(COLOR_TEXT);
      tft->setCursor(x + 4, centerY + 10);
      tft->print(F("D2:"));
      tft->print((int)data.disk2Load);
      tft->print(F("%"));
      
      tft->setTextColor(ST77XX_YELLOW);
      tft->setCursor(x + w - 28, centerY + 10);
      tft->print((int)data.disk2Temp);
      tft->print(F("C"));
    }
  }
  
}

// Helper function to draw combined Network tile (UP+DOWN in one tile)
void DisplayManager::drawTile_Network_Combined(int x, int y, int w, int h, const SystemData& data) {
  tft->drawRect(x, y, w, h, COLOR_NET);
  tft->setTextSize(1);
  tft->setTextColor(COLOR_NET);
  tft->setCursor(x + 2, y + 2);
  tft->print(F("NET"));
  
  int centerY = y + (h / 2) - 8;
  
  // Upload
  tft->setTextColor(ST77XX_GREEN);
  tft->setCursor(x + 2, centerY);
  tft->print(F("U:"));
  tft->setTextColor(COLOR_TEXT);
  if (data.netUp < 10) {
    tft->print(data.netUp, 1);
  } else {
    tft->print((int)data.netUp);
  }
  
  // Download
  tft->setTextColor(ST77XX_GREEN);
  tft->setCursor(x + 2, centerY + 10);
  tft->print(F("D:"));
  tft->setTextColor(COLOR_TEXT);
  if (data.netDown < 10) {
    tft->print(data.netDown, 1);
  } else {
    tft->print((int)data.netDown);
  }
  
  // Unit
  tft->setTextSize(1);
  tft->setTextColor(ST77XX_GREEN);
  tft->setCursor(x + w - 28, y + h - 10);
  tft->print(F("Mb/s"));
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
