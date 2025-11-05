/*
 * ESP8266 System Monitor với TFT Display 1.8" SPI
 * Hỗ trợ: ST7735 (128x160) hoặc ST7789 (135x240)
 * 
 * Đấu nối:
 * TFT CS  → D8 (GPIO15) - hoặc config.h
 * TFT RST → D4 (GPIO2)  - hoặc config.h
 * TFT DC  → D3 (GPIO0)  - hoặc config.h
 * TFT SDA → D7 (GPIO13) - Hardware SPI
 * TFT SCK → D5 (GPIO14) - Hardware SPI
 * TFT VCC → 3.3V
 * TFT GND → GND
 * TFT LED →  D2 (GPIO4) - Backlight control (PWM)
 * 
 * Button  → D1 (GPIO5) - hoặc config.h, nối GND (dùng pull-up internal)
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>

// Load config
#if __has_include("config.h")
  #include "config.h"
#else
  #error "config.h not found! Copy from config.h.example"
#endif

// Include thư viện TFT phù hợp
#ifdef TFT_ST7735
  #include <Adafruit_ST7735.h>
  Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
  #define SCREEN_WIDTH  128
  #define SCREEN_HEIGHT 160
#elif defined(TFT_ST7789)
  #include <Adafruit_ST7789.h>
  Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
  #define SCREEN_WIDTH  135
  #define SCREEN_HEIGHT 240
#else
  #error "Please define TFT_ST7735 or TFT_ST7789 in config.h"
#endif

// WiFi & Server
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
String serverUrl = String("http://") + SERVER_IP + ":" + SERVER_PORT + "/system-info";

WiFiClient wifiClient;

// Timing
unsigned long previousMillis = 0;
const long updateInterval = 3000; // Cập nhật mỗi 3 giây

// Button & Display control
bool displayOn = true;
bool lastButtonState = HIGH;  // Track button state
unsigned long lastButtonPress = 0;
unsigned long lastActivity = 0;
const long debounceDelay = 50;  // 50ms - chuẩn cho mechanical button

// Backlight control
#define BACKLIGHT_ON  255   // PWM 100% (full brightness)
#define BACKLIGHT_OFF 0     // PWM 0% (off)

// Colors (RGB565)
#define COLOR_BG       0x0000  // Black
#define COLOR_TEXT     0xFFFF  // White
#define COLOR_HEADER   0x07FF  // Cyan
#define COLOR_CPU      0xF800  // Red
#define COLOR_RAM      0x07E0  // Green
#define COLOR_GPU      0xFFE0  // Yellow
#define COLOR_DISK     0xF81F  // Magenta
#define COLOR_NET      0x001F  // Blue

// System data struct
struct SystemData {
  String cpuName;
  float cpuTemp, cpuLoad, cpuPower;
  float ramUsed, ramTotal, ramPercent;
  String gpuName;
  float gpuTemp, gpuLoad, gpuPower;
  int gpuMemUsed, gpuMemTotal;
  String disk1Name, disk2Name;
  float disk1Temp, disk1Load, disk2Temp, disk2Load;
  String netName;
  float netDown, netUp;
  bool hasData;
} sysData;

// Function prototypes
void initDisplay();
void handleButton();
void toggleDisplay();
void getSystemInfo();
void displaySystemInfo();
void drawHeader();
void drawCPU();
void drawRAM();
void drawGPU();
void drawDisk();
void drawNetwork();
void drawWiFiStatus();

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n\n=== ESP8266 System Monitor với TFT ===");
  
  // Setup LED backlight (PWM)
  Serial.println("Setting up LED backlight...");
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);  // Thử dùng digitalWrite thay vì analogWrite
  Serial.println("LED backlight ON");
  
  // Setup button (D1 = GPIO5, có internal pull-up)
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  delay(200);  // Đợi pin ổn định
  
  // Check button nhiều lần để đảm bảo
  for (int i = 0; i < 5; i++) {
    int state = digitalRead(BUTTON_PIN);
    Serial.print("Button check #");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(state == HIGH ? "HIGH" : "LOW");
    delay(50);
  }
  
  lastButtonState = digitalRead(BUTTON_PIN);  // Đọc trạng thái ban đầu
  Serial.print("==> Final button state: ");
  Serial.println(lastButtonState == HIGH ? "HIGH (OK - not pressed)" : "LOW (ERROR - check wiring!)");
  
  // Init display
  Serial.println("Initializing TFT display...");
  initDisplay();
  Serial.println("TFT display initialized!");
  
  // Hiển thị splash screen
  Serial.println("Drawing splash screen...");
  tft.fillScreen(ST77XX_RED);  // Test màu đỏ trước
  delay(500);
  tft.fillScreen(ST77XX_GREEN);  // Test màu xanh
  delay(500);
  tft.fillScreen(ST77XX_BLUE);  // Test màu xanh dương
  delay(500);
  
  tft.fillScreen(COLOR_BG);
  tft.setTextColor(COLOR_HEADER);
  tft.setTextSize(2);
  tft.setCursor(10, 60);
  tft.println("System");
  tft.setCursor(10, 80);
  tft.println("Monitor");
  tft.setTextSize(1);
  tft.setCursor(10, 110);
  tft.setTextColor(COLOR_TEXT);
  tft.println("Connecting WiFi...");
  Serial.println("Splash screen drawn!");
  
  // Kết nối WiFi
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    
    tft.fillRect(0, 110, SCREEN_WIDTH, 20, COLOR_BG);
    tft.setCursor(10, 110);
    tft.setTextColor(COLOR_HEADER);
    tft.println("WiFi OK!");
    delay(1000);
  } else {
    Serial.println("\nWiFi failed!");
    tft.fillRect(0, 110, SCREEN_WIDTH, 20, COLOR_BG);
    tft.setCursor(10, 110);
    tft.setTextColor(COLOR_CPU);
    tft.println("WiFi Failed!");
    delay(2000);
  }
  
  tft.fillScreen(COLOR_BG);
  lastActivity = millis();  // Reset timer sau khi boot xong
  previousMillis = millis();  // Reset update timer
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Check button (bỏ qua 5 giây đầu để tránh false trigger khi boot)
  if (currentMillis > 5000) {
    handleButton();
  }
  
  // DISABLE auto turn off - chỉ dùng button thủ công
  // if (displayOn && sysData.hasData && (currentMillis - lastActivity > BACKLIGHT_TIMEOUT)) {
  //   toggleDisplay();
  // }
  
  // Check WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost! Reconnecting...");
    WiFi.reconnect();
    delay(5000);
    return;
  }
  
  // Update data
  if (displayOn && currentMillis - previousMillis >= updateInterval) {
    previousMillis = currentMillis;
    getSystemInfo();
    if (sysData.hasData) {
      displaySystemInfo();
      lastActivity = currentMillis;
    }
  }
}

void initDisplay() {
  #ifdef TFT_ST7735
    tft.initR(INITR_BLACKTAB);  // Hoặc INITR_GREENTAB, INITR_REDTAB tùy màn
  #elif defined(TFT_ST7789)
    tft.init(SCREEN_WIDTH, SCREEN_HEIGHT);
  #endif
  
  tft.setRotation(SCREEN_ROTATION);
  tft.fillScreen(COLOR_BG);
  Serial.println("TFT initialized!");
}

void handleButton() {
  // Đọc button state hiện tại
  int reading = digitalRead(BUTTON_PIN);
  
  // Debug: In state mỗi khi có thay đổi
  if (reading != lastButtonState) {
    Serial.print("Button state changed: ");
    Serial.print(lastButtonState == HIGH ? "HIGH" : "LOW");
    Serial.print(" -> ");
    Serial.println(reading == HIGH ? "HIGH" : "LOW");
  }
  
  // Detect falling edge (HIGH → LOW = button pressed)
  if (reading == LOW && lastButtonState == HIGH) {
    unsigned long now = millis();
    if (now - lastButtonPress > debounceDelay) {
      lastButtonPress = now;
      Serial.println("=== BUTTON PRESSED - TOGGLING DISPLAY ===");
      toggleDisplay();
    } else {
      Serial.println("Button press ignored (debounce)");
    }
  }
  
  lastButtonState = reading;
}

void toggleDisplay() {
  displayOn = !displayOn;
  Serial.print("toggleDisplay() called. New state: ");
  Serial.println(displayOn ? "ON" : "OFF");
  
  if (displayOn) {
    // Bật màn hình
    digitalWrite(TFT_LED, HIGH);  // Bật LED backlight
    Serial.println("  - LED backlight: HIGH");
    tft.fillScreen(COLOR_BG);
    lastActivity = millis();
    Serial.println("  - Display ON complete");
  } else {
    // Tắt màn hình
    digitalWrite(TFT_LED, LOW);  // Tắt LED backlight (tiết kiệm điện)
    Serial.println("  - LED backlight: LOW");
    tft.fillScreen(COLOR_BG);
    Serial.println("  - Display OFF complete");
  }
}

void getSystemInfo() {
  HTTPClient http;
  
  http.begin(wifiClient, serverUrl.c_str());
  http.setTimeout(5000);
  
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      // Parse CPU
      sysData.cpuName = doc["cpu"]["name"].as<String>();
      sysData.cpuTemp = doc["cpu"]["temp"].as<float>();
      sysData.cpuLoad = doc["cpu"]["load"].as<float>();
      sysData.cpuPower = doc["cpu"]["power"].as<float>();
      
      // Parse RAM
      sysData.ramUsed = doc["ram"]["used"].as<float>();
      sysData.ramTotal = doc["ram"]["total"].as<float>();
      sysData.ramPercent = doc["ram"]["percent"].as<float>();
      
      // Parse GPU
      sysData.gpuName = doc["gpu_discrete"]["name"].as<String>();
      sysData.gpuTemp = doc["gpu_discrete"]["temp"].as<float>();
      sysData.gpuLoad = doc["gpu_discrete"]["load"].as<float>();
      sysData.gpuPower = doc["gpu_discrete"]["power"].as<float>();
      sysData.gpuMemUsed = doc["gpu_discrete"]["mem_used"].as<int>();
      sysData.gpuMemTotal = doc["gpu_discrete"]["mem_total"].as<int>();
      
      // Parse Disks
      JsonArray disks = doc["disk"].as<JsonArray>();
      if (disks.size() > 0) {
        sysData.disk1Name = disks[0]["name"].as<String>();
        sysData.disk1Temp = disks[0]["temp"].as<float>();
        sysData.disk1Load = disks[0]["load"].as<float>();
      }
      if (disks.size() > 1) {
        sysData.disk2Name = disks[1]["name"].as<String>();
        sysData.disk2Temp = disks[1]["temp"].as<float>();
        sysData.disk2Load = disks[1]["load"].as<float>();
      }
      
      // Parse Network
      sysData.netName = doc["network"]["name"].as<String>();
      sysData.netDown = doc["network"]["download"].as<float>();
      sysData.netUp = doc["network"]["upload"].as<float>();
      
      sysData.hasData = true;
      Serial.println("Data updated!");
    } else {
      Serial.println("JSON parse error!");
      sysData.hasData = false;
    }
  } else {
    Serial.printf("HTTP error: %d\n", httpCode);
    sysData.hasData = false;
  }
  
  http.end();
}

void displaySystemInfo() {
  tft.fillScreen(COLOR_BG);
  
  int y = 2;
  
  // Header
  tft.setTextSize(1);
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(2, y);
  tft.print("SYSTEM MONITOR");
  y += 12;
  
  // CPU
  tft.setTextColor(COLOR_CPU);
  tft.setCursor(2, y);
  tft.print("CPU:");
  tft.setTextColor(COLOR_TEXT);
  tft.print(String(sysData.cpuTemp, 0) + "C ");
  tft.print(String(sysData.cpuLoad, 0) + "%");
  y += 10;
  
  // RAM
  tft.setTextColor(COLOR_RAM);
  tft.setCursor(2, y);
  tft.print("RAM:");
  tft.setTextColor(COLOR_TEXT);
  tft.print(String(sysData.ramUsed, 1) + "/" + String(sysData.ramTotal, 1) + "GB");
  y += 10;
  
  // GPU
  if (sysData.gpuName.length() > 0) {
    tft.setTextColor(COLOR_GPU);
    tft.setCursor(2, y);
    tft.print("GPU:");
    tft.setTextColor(COLOR_TEXT);
    tft.print(String(sysData.gpuTemp, 0) + "C ");
    tft.print(String(sysData.gpuLoad, 0) + "%");
    y += 10;
    
    if (sysData.gpuMemTotal > 0) {
      tft.setCursor(2, y);
      tft.print("VRAM:" + String(sysData.gpuMemUsed) + "/" + String(sysData.gpuMemTotal) + "MB");
      y += 10;
    }
  }
  
  // Disk 1
  if (sysData.disk1Name.length() > 0) {
    tft.setTextColor(COLOR_DISK);
    tft.setCursor(2, y);
    tft.print("SSD1:");
    tft.setTextColor(COLOR_TEXT);
    tft.print(String(sysData.disk1Temp, 0) + "C ");
    tft.print(String(sysData.disk1Load, 0) + "%");
    y += 10;
  }
  
  // Disk 2
  if (sysData.disk2Name.length() > 0) {
    tft.setTextColor(COLOR_DISK);
    tft.setCursor(2, y);
    tft.print("SSD2:");
    tft.setTextColor(COLOR_TEXT);
    tft.print(String(sysData.disk2Temp, 0) + "C ");
    tft.print(String(sysData.disk2Load, 0) + "%");
    y += 10;
  }
  
  // Network
  if (sysData.netName.length() > 0) {
    tft.setTextColor(COLOR_NET);
    tft.setCursor(2, y);
    tft.print("NET:");
    tft.setTextColor(COLOR_TEXT);
    tft.print("D:" + String(sysData.netDown, 1) + " U:" + String(sysData.netUp, 1));
    y += 10;
  }
  
  // WiFi status
  tft.setCursor(2, SCREEN_HEIGHT - 10);
  tft.setTextColor(COLOR_HEADER);
  tft.print("WiFi: ");
  tft.print(WiFi.localIP().toString());
}
