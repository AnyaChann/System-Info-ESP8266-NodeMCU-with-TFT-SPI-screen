# 📺 Hướng dẫn Setup TFT Display 1.8" SPI

## 🔌 Sơ đồ đấu nối

### TFT Display (ST7735 128x160 hoặc ST7789 135x240)

```
TFT Pin    →  ESP8266 NodeMCU
--------------------------------
VCC        →  3.3V
GND        →  GND
CS         →  D8 (GPIO15)
RESET      →  D4 (GPIO2)
DC/A0      →  D3 (GPIO0)
SDA/MOSI   →  D7 (GPIO13) - Hardware SPI
SCK/SCL    →  D5 (GPIO14) - Hardware SPI
LED        →  D2 (GPIO4) - Backlight control (PWM)
```

### Button (Bật/Tắt màn hình)

```
Button Pin  →  ESP8266
-----------------------
One side    →  D1 (GPIO5)
Other side  →  GND
```
**Note:** Dùng INPUT_PULLUP internal, không cần resistor ngoài.

---

## ⚙️ Cấu hình

### 1. Cập nhật `platformio.ini`
Đã được thêm tự động:
```ini
lib_deps = 
    bblanchon/ArduinoJson@^6.21.3
    adafruit/Adafruit ST7735 and ST7789 Library@^1.10.0
    adafruit/Adafruit GFX Library@^1.11.3
```

### 2. Cập nhật `include/config.h`

Copy từ `config.h.example` và chỉnh sửa:

```cpp
// ===== TFT Display Configuration =====
// Uncomment loại màn hình bạn đang dùng:
#define TFT_ST7735    // 128x160 (phổ biến nhất)
// #define TFT_ST7789    // 135x240

// Chân kết nối TFT
#define TFT_CS    D8   // GPIO 15
#define TFT_RST   D4   // GPIO 2
#define TFT_DC    D3   // GPIO 0
#define TFT_LED   D2   // GPIO 4 - Backlight control

// Button Configuration
#define BUTTON_PIN D1  // GPIO 5

// Display Settings
#define SCREEN_ROTATION 1  // 0-3 (xoay màn hình)
#define BACKLIGHT_TIMEOUT 60000  // Tự tắt sau 60s (ms)
```

### 3. Sử dụng code mới

**Option A: Rename file (khuyến nghị)**
```bash
# Backup code cũ
mv src/main.cpp src/main_serial_only.cpp.bak

# Dùng code có TFT
mv src/main_with_tft.cpp.example src/main.cpp
```

**Option B: Copy nội dung**
Copy toàn bộ nội dung từ `main_with_tft.cpp.example` → `main.cpp`

---

## 🎨 Tùy chỉnh màn hình

### Xoay màn hình
Trong `config.h`:
```cpp
#define SCREEN_ROTATION 0  // 0° (Portrait)
#define SCREEN_ROTATION 1  // 90° (Landscape)
#define SCREEN_ROTATION 2  // 180° (Portrait inverted)
#define SCREEN_ROTATION 3  // 270° (Landscape inverted)
```

### Thay đổi màu sắc
Trong `main.cpp`, tìm và sửa:
```cpp
#define COLOR_CPU      0xF800  // Red
#define COLOR_RAM      0x07E0  // Green
#define COLOR_GPU      0xFFE0  // Yellow
// ... (RGB565 format)
```

### Chọn đúng loại màn hình ST7735
Nếu màn hình bị lỗi màu, thay đổi dòng này:
```cpp
tft.initR(INITR_BLACKTAB);  // Thử: BLACKTAB, GREENTAB, REDTAB
```

---

## 🔧 Build & Upload

```bash
# Install dependencies
pio lib install

# Build
pio run

# Upload
pio run --target upload

# Monitor
pio device monitor
```

---

## 🎮 Sử dụng

1. **Bật nguồn:** Màn hình hiển thị splash screen (3 màu test) → "System Monitor" → kết nối WiFi
2. **Tự động cập nhật:** Dữ liệu refresh mỗi 3 giây
3. **Bấm nút D1:** Bật/tắt backlight màn hình
4. **Auto-off:** Đã disable - chỉ dùng button để tắt/bật

---

## 🐛 Troubleshooting

### Màn hình trắng/không hiển thị
1. Kiểm tra đấu nối CS, RST, DC
2. Thử đổi `INITR_BLACKTAB` → `INITR_GREENTAB` hoặc `INITR_REDTAB`
3. Kiểm tra 3.3V đủ nguồn (dùng nguồn ngoài nếu USB yếu)

### Màu sắc sai
- Đổi loại init trong code:
  ```cpp
  tft.initR(INITR_GREENTAB);  // Hoặc REDTAB
  ```

### Button không hoạt động
- Kiểm tra đấu nối D1 → GND
- Button phải là loại normally open (NO)
- Nếu vẫn không hoạt động, thêm resistor pull-up 10kΩ từ D1 → 3.3V

### WiFi không kết nối
- Kiểm tra `config.h` có đúng SSID/Password
- Kiểm tra server Python đang chạy

---

## 📊 Hiển thị trên màn hình

```
┌─────────────────────┐
│ SYSTEM MONITOR      │
│ CPU: 75°C 45%       │
│ RAM: 13.2/16.0GB    │
│ GPU: 52°C 30%       │
│ VRAM: 2048/6144MB   │
│ SSD1: 43°C 68%      │
│ SSD2: 42°C 85%      │
│ NET: D:1.2 U:0.5    │
│                     │
│ WiFi: 192.168.2.x   │
└─────────────────────┘
```

---

## 🚀 Next Steps

- [ ] Thêm biểu đồ real-time
- [ ] Thêm nhiều trang (button để chuyển)
- [ ] Điều khiển độ sáng LED backlight
- [ ] Custom font đẹp hơn
- [ ] Icons cho từng sensor

