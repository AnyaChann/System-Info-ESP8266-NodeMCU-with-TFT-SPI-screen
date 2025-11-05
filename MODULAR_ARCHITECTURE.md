# 📁 Modular Architecture - System Monitor

## 🎯 Tổng quan

Code đã được tách thành các module độc lập để dễ bảo trì và mở rộng.

## 📂 Cấu trúc thư mục

```
System Info ESP8266 NodeMCU/
├── include/                      # Header files (.h)
│   ├── config.h                  # WiFi, Server, TFT pins config
│   ├── system_data.h             # Data structures & color definitions
│   ├── display_manager.h         # TFT display interface
│   ├── network_manager.h         # WiFi & HTTP interface
│   └── button_handler.h          # Button input interface
│
├── src/                          # Implementation files (.cpp)
│   ├── main.cpp                  # Main program (setup & loop)
│   ├── display_manager.cpp       # TFT display implementation
│   ├── network_manager.cpp       # WiFi & HTTP implementation
│   └── button_handler.cpp        # Button handler implementation
│
└── lib/
    └── components/               # (Empty - files moved to src/)
```

## 🔧 Các Module

### 1️⃣ **SystemData** (`system_data.h`)
- **Mục đích**: Định nghĩa struct chứa data từ server
- **Nội dung**:
  - `SystemData` struct (CPU, RAM, GPU, Disk, Network)
  - Color constants (RGB565)
- **Không cần .cpp**: Chỉ là data definitions

### 2️⃣ **DisplayManager** (`display_manager.h` + `.cpp`)
- **Mục đích**: Quản lý TFT display
- **Chức năng**:
  - `begin()` - Khởi tạo display
  - `showSplashScreen()` - Màn hình khởi động
  - `displaySystemInfo()` - Hiển thị system data
  - `toggle()` - Bật/tắt màn hình
  - `turnOn()` / `turnOff()` - Điều khiển backlight
  - `isOn()` - Kiểm tra trạng thái
- **Dependencies**: 
  - Adafruit_ST7735/ST7789
  - Adafruit_GFX
  - ESP8266WiFi (cho WiFi.localIP())

### 3️⃣ **NetworkManager** (`network_manager.h` + `.cpp`)
- **Mục đích**: Quản lý WiFi và HTTP requests
- **Chức năng**:
  - `connectWiFi()` - Kết nối WiFi
  - `isConnected()` - Kiểm tra kết nối
  - `reconnect()` - Kết nối lại khi mất
  - `fetchSystemData()` - Lấy data từ server
  - `shouldUpdate()` - Kiểm tra timing update
  - `getLocalIP()` - Lấy địa chỉ IP
- **Dependencies**:
  - ESP8266WiFi
  - ESP8266HTTPClient
  - ArduinoJson

### 4️⃣ **ButtonHandler** (`button_handler.h` + `.cpp`)
- **Mục đích**: Xử lý button input
- **Chức năng**:
  - `begin()` - Khởi tạo button pin
  - `update()` - Đọc và xử lý button state
  - `setCallback()` - Đăng ký hàm callback
  - `isPressed()` - Kiểm tra button có đang nhấn
- **Tính năng**:
  - Debounce 50ms
  - Edge detection (HIGH→LOW)
  - Callback pattern

### 5️⃣ **Main** (`main.cpp`)
- **Mục đích**: Entry point của chương trình
- **Code tối giản**: Chỉ ~60 dòng (so với 350+ dòng trước)
- **Logic flow**:
  ```cpp
  setup() {
    display.begin()
    button.begin()
    network.connectWiFi()
  }
  
  loop() {
    button.update()
    if (network.shouldUpdate()) {
      network.fetchSystemData()
      display.displaySystemInfo()
    }
  }
  ```

## ✨ Lợi ích của Modular Architecture

### 🧩 **Separation of Concerns**
- Mỗi module có trách nhiệm riêng
- Dễ hiểu chức năng từng phần
- Không còn "God class" 350+ dòng

### 🔧 **Dễ bảo trì**
- Sửa bug chỉ cần vào 1 file
- Không ảnh hưởng code khác
- Dễ test từng module riêng

### 🚀 **Dễ mở rộng**
- Thêm feature mới không cần động main.cpp
- Có thể tái sử dụng module cho project khác
- Dễ thêm multiple displays, buttons, sensors

### 📚 **Dễ đọc & học**
- Header file = interface documentation
- Code tự giải thích (self-documenting)
- Dễ onboard developer mới

## 🔄 So sánh với code cũ

### ❌ **Trước (Monolithic):**
```cpp
// main.cpp - 350+ dòng
void setup() {
  // 100+ dòng init code
}

void loop() {
  // 50+ dòng logic
}

void getSystemInfo() { /* 80 dòng */ }
void displaySystemInfo() { /* 100 dòng */ }
void handleButton() { /* 30 dòng */ }
```

### ✅ **Sau (Modular):**
```cpp
// main.cpp - 60 dòng
DisplayManager display(...);
NetworkManager network(...);
ButtonHandler button(...);

void setup() {
  display.begin();
  button.begin();
  network.connectWiFi();
}

void loop() {
  button.update();
  if (network.shouldUpdate())
    display.displaySystemInfo(...);
}
```

## 📝 Backup files

- `src/main_monolithic.cpp.bak` - Backup code cũ (350+ dòng)
- `src/main_serial_only.cpp.bak` - Version serial-only ban đầu

## 🛠️ Cách sử dụng

### **Thêm tính năng mới**

#### Ví dụ 1: Thêm second button
```cpp
// include/button_handler.h - Không cần sửa
// src/main.cpp
ButtonHandler button1(D1);
ButtonHandler button2(D6);

void onButton1Pressed() { /* ... */ }
void onButton2Pressed() { /* ... */ }

void setup() {
  button1.setCallback(onButton1Pressed);
  button2.setCallback(onButton2Pressed);
}

void loop() {
  button1.update();
  button2.update();
}
```

#### Ví dụ 2: Thêm OLED display
```cpp
// Tạo OLEDManager tương tự DisplayManager
// include/oled_manager.h
class OLEDManager { /* ... */ };

// src/main.cpp
DisplayManager tft(...);
OLEDManager oled(...);

tft.displaySystemInfo(data);
oled.displaySystemInfo(data);  // Cùng data, khác output
```

#### Ví dụ 3: Thêm MQTT publisher
```cpp
// include/mqtt_manager.h
class MQTTManager {
  void publish(const SystemData& data);
};

// src/main.cpp
MQTTManager mqtt(...);
mqtt.publish(sysData);  // Gửi data lên MQTT broker
```

## 🎓 Best Practices

### ✅ **DO:**
- Thêm method mới vào class tương ứng
- Tạo manager mới cho feature phức tạp
- Sử dụng callback cho event handling
- Giữ main.cpp nhỏ gọn

### ❌ **DON'T:**
- Viết logic vào main.cpp
- Mix concerns giữa các module
- Truy cập trực tiếp vào private members
- Tạo global variables bừa bãi

## 📊 Memory Usage

**Modular vs Monolithic:** ~Same size!
```
RAM:   36.1% (29,552 bytes)
Flash: 29.4% (307,327 bytes)
```
→ **Không tốn thêm memory**, chỉ tổ chức code tốt hơn!

## 🔍 Troubleshooting

### Compile error: "undefined reference"
→ Kiểm tra file `.cpp` có trong `src/` không

### Include error: "file not found"
→ Thêm `#include "../include/your_header.h"` nếu cần

### Multiple definition error
→ Kiểm tra header guard (`#ifndef`, `#define`, `#endif`)

## 📚 Tài liệu thêm

- [C++ Class Design](https://isocpp.org/wiki/faq/classes-and-objects)
- [SOLID Principles](https://en.wikipedia.org/wiki/SOLID)
- [PlatformIO Library Management](https://docs.platformio.org/en/latest/librarymanager/)

---

**Created by**: AnyaChann  
**Date**: 2025-11-05  
**Version**: 2.0.0 - Modular Architecture
