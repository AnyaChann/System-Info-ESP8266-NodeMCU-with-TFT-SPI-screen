# Quick Start - 3 phút

## 📋 Yêu cầu

- ESP8266 NodeMCU
- Libre Hardware Monitor
- Python 3.7+
- WiFi 2.4GHz

## ⚡ Bước 1: Cài đặt Libre Hardware Monitor (2 phút)

1. Download: https://github.com/LibreHardwareMonitor/LibreHardwareMonitor/releases
2. Giải nén và chạy `LibreHardwareMonitor.exe` (với quyền Administrator)
3. Bật Remote Web Server:
   - Options → Remote Web Server → Enable
   - Port: 8085 (default)

✅ Test: Mở trình duyệt → `http://localhost:8085/data.json` → Thấy JSON data

## ⚡ Bước 2: Cấu hình (1 phút)

**A. Python Server (server/.env):**
```powershell
cd server
copy .env.example .env
notepad .env
```

**B. ESP8266 (include/config.h):**
```powershell
cd ..\include
copy config.h.example config.h
notepad config.h
```

Sửa WiFi, IP và Port:
```cpp
#define WIFI_SSID "TenWiFi"
#define WIFI_PASSWORD "MatKhau"
#define SERVER_IP "192.168.2.60"
#define SERVER_PORT "8080"
```

## ⚡ Bước 3: Start server (30 giây)

```powershell
cd "d:\PlatformIO IDE\System Info ESP8266 NodeMCU"
pip install -r server/requirements.txt
python server/system_monitor_server.py
```

✅ Server chạy tại: `http://192.168.2.60:8080/system-info`

## ⚡ Bước 4: Upload & Chạy (30 giây)

```powershell
cd "d:\PlatformIO IDE\System Info ESP8266 NodeMCU"
pio run --target upload
pio device monitor
```

✅ Xem thông tin hệ thống hiển thị trên Serial Monitor!

## 🎛️ Tùy chọn: Bật/tắt Debug

Chỉnh file `server/.env`:

```env
DEBUG_MODE=true   # Hiển thị log chi tiết
DEBUG_MODE=false  # Chỉ hiển thị thông tin cần thiết
```

## 🆘 Lỗi thường gặp

1. **ESP8266 không kết nối WiFi** → Kiểm tra `config.h` và WiFi 2.4GHz
2. **Dữ liệu = 0** → Kiểm tra Libre HW Monitor đang chạy
3. **Module not found** → `pip install -r server/requirements.txt`
4. **Compile error** → Kiểm tra đã có `include/config.h`

## 📊 Kết quả mong đợi

```
========================================
     SYSTEM MONITOR - ESP8266
========================================

[CPU]
  AMD Ryzen 7 5800H with Radeon Graphics
  Temp: 60.6°C | Load: 7.0% | Power: 6.3W

[RAM]
  12.8 / 15.3 GB (83.5%)

[GPU - Discrete]
  NVIDIA GeForce RTX 3060 Laptop GPU
  Temp: 46.0°C | Load: 0.0% | Power: 20.7W
  VRAM: 148 / 6144 MB

[iGPU - Integrated]
  AMD Radeon(TM) Graphics
  Temp: N/A | Load: 6.0%

[STORAGE]
  SAMSUNG MZVLQ512HBLU-00BH1
  Temp: 38.0°C | Used: 78.0%

[NETWORK]
  Qualcomm QCA61x4A Wireless
  Upload: 398.9 KB/s | Download: 9.3 KB/s
========================================
```

## 🚀 Tiếp theo

- Đọc [README.md](README.md) để hiểu đầy đủ tính năng
- Đọc [HARDWARE_SUPPORT.md](server/HARDWARE_SUPPORT.md) để xem phần cứng được hỗ trợ
- Tùy chỉnh trong file `.env`

## 📝 Lưu ý

- File `config.h` được ignore bởi git (chứa WiFi password)
- Thay đổi config → Chỉnh `config.h` và `.env` → Upload lại
- Debug mode: Sửa `DEBUG_MODE=true` trong `.env`

---

**Thời gian:** ~3 phút ⚡
