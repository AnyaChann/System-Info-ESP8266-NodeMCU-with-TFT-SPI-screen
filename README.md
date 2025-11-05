# ESP8266 System Monitor

Firmware cho ESP8266 NodeMCU để hiển thị thông tin hệ thống PC qua WiFi, lấy dữ liệu từ Libre Hardware Monitor.

## Tính năng

✅ **Phát hiện phần cứng linh hoạt** - Hỗ trợ nhiều hãng và cấu hình:
- **CPU**: Intel (Core, Xeon, Pentium, Celeron), AMD (Ryzen, EPYC, Athlon)
- **GPU rời**: NVIDIA (GeForce, GTX, RTX, Quadro), AMD (Radeon RX/PRO), Intel Arc
- **iGPU**: AMD Radeon Graphics, Intel UHD/Iris/HD Graphics
- **RAM**: Tự động phát hiện
- **Disk/SSD**: Samsung, WD, Seagate, Toshiba, Kingston, Crucial, SanDisk, Intel, Micron, Hynix
- **Network**: Wi-Fi, Ethernet, Realtek, Intel, Qualcomm, Broadcom

✅ **Dữ liệu chi tiết**:
- CPU: Tên, nhiệt độ, load, công suất
- RAM: Sử dụng, tổng, phần trăm
- GPU rời: Tên, nhiệt độ, load, công suất, VRAM
- iGPU: Tên, nhiệt độ (nếu có), load
- Disk: Tên, nhiệt độ, dung lượng đã dùng
- Network: Tên, tốc độ upload/download

✅ **Tối ưu cho ESP8266**:
- JSON size tối ưu (< 2KB)
- Giới hạn tối đa 2 disk
- Chỉ hiển thị network adapter đang active

## Yêu cầu

### Phần cứng
- ESP8266 NodeMCU (hoặc board tương tự)
- PC chạy Windows với Libre Hardware Monitor

### Phần mềm
- **PC**:
  - [Libre Hardware Monitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor) (phiên bản mới nhất)
  - Python 3.7+ với pip
  
- **ESP8266**:
  - PlatformIO IDE (hoặc Arduino IDE)
  - ArduinoJson library v6.21.3

## Cài đặt

### Bước 1: Cài đặt Libre Hardware Monitor

1. Tải và cài đặt [Libre Hardware Monitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor/releases)
2. Chạy với quyền Administrator
3. Bật Remote Web Server:
   - Options → Remote Web Server
   - Port: 8085 (mặc định)

### Bước 2: Cấu hình

**A. Cấu hình Python Server (`server/.env`):**

Chỉnh sửa file `.env` trong folder `server`:

```powershell
cd server
copy .env.example .env
notepad .env
```

Nội dung:
```env
# IP của PC (để trống = tự động phát hiện)
PC_IP_ADDRESS=

# Server config
DEBUG_MODE=true
SERVER_PORT=8080
LIBRE_HW_MONITOR_PORT=8085
MAX_DISKS=2
```

**B. Cấu hình ESP8266 (`include/config.h`):**

Copy và chỉnh sửa:

```powershell
cd ..\include
copy config.h.example config.h
notepad config.h
```

Sửa WiFi, IP và Port trong `config.h`:
```cpp
#define WIFI_SSID "TenWiFiCuaBan"
#define WIFI_PASSWORD "MatKhauWiFi"
#define SERVER_IP "192.168.0.0"     // IP từ .env hoặc tự động phát hiện
#define SERVER_PORT "8080"
```

### Bước 3: Cài đặt và chạy Python Server

```powershell
cd server
pip install -r requirements.txt
cd ..
python server/system_monitor_server.py
```

Server sẽ chạy tại `http://<IP_máy_bạn>:8080`

### Bước 4: Upload firmware

```powershell
cd "d:\PlatformIO IDE\System Info ESP8266 NodeMCU"
pio run --target upload
pio device monitor
```

## Kết quả mẫu

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
  SAMSUNG MZVL21T0HCLR-00B00
  Temp: 36.0°C | Used: 44.0%

[NETWORK]
  Qualcomm QCA61x4A Wireless Net...
  Upload: 398.9 KB/s | Download: 9.3 KB/s

========================================
```

## Cấu trúc thư mục

```
System Info ESP8266 NodeMCU/
├── src/
│   └── main.cpp              # Firmware ESP8266
├── server/
│   ├── system_monitor_server.py   # Python Flask server
│   └── requirements.txt           # Python dependencies
├── platformio.ini            # PlatformIO config
└── README.md                # File này
```

## API Endpoint

### GET /system-info

Trả về JSON chứa thông tin hệ thống:

```json
{
  "cpu": {
    "name": "AMD Ryzen 7 5800H with Radeon Graphics",
    "temp": 60.6,
    "load": 7.0,
    "power": 6.3
  },
  "ram": {
    "used": 12.8,
    "total": 15.3,
    "percent": 83.5
  },
  "gpu_discrete": {
    "name": "NVIDIA GeForce RTX 3060 Laptop GPU",
    "temp": 46.0,
    "load": 0.0,
    "power": 20.7,
    "mem_used": 148,
    "mem_total": 6144
  },
  "gpu_integrated": {
    "name": "AMD Radeon(TM) Graphics",
    "temp": 0,
    "load": 6.0
  },
  "disk": [
    {
      "name": "SAMSUNG MZVLQ512HBLU-00BH1",
      "temp": 38.0,
      "load": 78.0
    }
  ],
  "network": {
    "name": "Qualcomm QCA61x4A Wireless Network Adapter",
    "upload": 398.9,
    "download": 9.3
  }
}
```

## Khắc phục sự cố

### Python Server không chạy
- Kiểm tra Libre Hardware Monitor đang chạy
- Kiểm tra Remote Web Server enabled (port 8085)
- Thử truy cập: `http://localhost:8085/data.json`
- Bật debug mode trong `.env`: `DEBUG_MODE=true`

### ESP8266 không kết nối WiFi
- Kiểm tra SSID và password trong `include/config.h`
- Kiểm tra WiFi 2.4GHz (ESP8266 không hỗ trợ 5GHz)
- Đảm bảo đã copy `config.h.example` thành `config.h`

### Dữ liệu trả về 0
- Kiểm tra sensor trong Libre Hardware Monitor
- Một số sensor có thể không khả dụng trên phần cứng của bạn
- iGPU thường không có sensor nhiệt độ riêng

### JSON quá lớn cho ESP8266
- Server tự động giới hạn 2 disk
- Chỉ lấy network adapter có traffic
- Buffer size: 2048 bytes (có thể tăng nếu cần)

## Tùy chỉnh

### Thay đổi tần suất cập nhật

Trong `main.cpp`:
```cpp
delay(5000);  // Thay đổi 5000 (5 giây) thành giá trị khác
```

### Tăng buffer size

Trong `main.cpp`:
```cpp
DynamicJsonDocument doc(2048);  // Tăng 2048 nếu cần
```

### Thêm nhiều disk

Trong `system_monitor_server.py`:
```python
result["disk"] = result["disk"][:2]  # Thay 2 thành số lượng mong muốn
```

## Giấy phép

MIT License - Sử dụng tự do cho mục đích cá nhân và thương mại.

## Tác giả

Được phát triển bởi GitHub Copilot cho ESP8266 NodeMCU.

## Đóng góp

Mọi đóng góp đều được chào đón! Hãy tạo issue hoặc pull request.
