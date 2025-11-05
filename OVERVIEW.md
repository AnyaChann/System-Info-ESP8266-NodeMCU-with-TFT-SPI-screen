# 📦 System Info ESP8266 NodeMCU

Monitor PC hardware info on ESP8266 via WiFi.

## 🚀 Quick Setup

1. **Config files:**
   ```bash
   # Python Server
   cd server
   copy .env.example .env
   notepad .env
   
   # ESP8266
   cd ..\include
   copy config.h.example config.h
   notepad config.h
   ```

2. **Start server:**
   ```bash
   pip install -r server/requirements.txt
   python server/system_monitor_server.py
   ```

3. **Upload ESP8266:**
   ```bash
   pio run --target upload
   pio device monitor
   ```

## 📁 File Structure

```
include/
  config.h              ← ESP8266 config (copy từ .example)
  config.h.example      ← Template
src/main.cpp           ← ESP8266 firmware
server/
  .env                  ← Python server config
  system_monitor_server.py
```

## ⚙️ Config

**.env** (Python Server):
```env
PC_IP_ADDRESS=          # Trống = auto detect
DEBUG_MODE=true
SERVER_PORT=8080
```

**include/config.h** (ESP8266):
```cpp
#define WIFI_SSID "YourWiFi"
#define WIFI_PASSWORD "YourPassword"
#define SERVER_IP "192.168.2.60"
#define SERVER_PORT "8080"
```

## 📊 Features

- CPU: Temp, Load, Power
- RAM: Used, Total, %
- GPU: Temp, Load, Power, VRAM
- Disk: Temp, Used %
- Network: Upload/Download speed

## 📖 Docs

- **README.md** - Full documentation
- **QUICKSTART.md** - 3-minute setup guide
- **CHANGELOG.md** - Version history

## 🔧 Requirements

- ESP8266 NodeMCU
- Libre Hardware Monitor (PC)
- Python 3.7+
- WiFi 2.4GHz

---

**Version:** 1.2.0 | **Time:** ~3 minutes ⚡
