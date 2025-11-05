# 📡 OTA (Over-The-Air) Update Guide

## 🎯 Tổng quan

OTA cho phép update firmware ESP8266 qua WiFi, không cần cắm USB cable!

### ✅ Lợi ích:
- Update từ xa (không cần physical access)
- Tiện lợi khi ESP8266 đã lắp đặt cố định
- Nhanh hơn (~20 giây vs ~25 giây qua USB)
- An toàn (có password bảo vệ)

---

## 🔧 Cài đặt

### **1. Config trong `config.h`:**

```cpp
// OTA Configuration
#define OTA_ENABLED true              // Enable/disable OTA updates
#define OTA_HOSTNAME "ESP8266-Monitor" // Hostname for OTA
#define OTA_PASSWORD ""               // OTA password (empty = no password)
```

**Khuyến nghị:**
- Set `OTA_PASSWORD` để bảo mật: `#define OTA_PASSWORD "your_secure_password"`
- Change `OTA_HOSTNAME` để dễ nhận diện: `#define OTA_HOSTNAME "ESP8266-Living-Room"`

### **2. Upload lần đầu qua USB:**

```bash
platformio run --target upload --environment esp12e
```

⚠️ **Lần đầu PHẢI upload qua USB** để enable OTA support!

---

## 🚀 Cách sử dụng OTA

### **Method 1: PlatformIO (Recommended)**

#### **Bước 1: Kiểm tra ESP8266 trên network**

Sau khi upload, ESP8266 sẽ xuất hiện trên network với hostname `ESP8266-Monitor` (hoặc tên bạn đặt).

#### **Bước 2: Upload qua OTA**

**Option A: Tự động detect**
```bash
platformio run --target upload --environment esp12e
```
PlatformIO sẽ tự động detect ESP8266 trên network và hỏi bạn chọn:
```
Available OTA devices:
[1] ESP8266-Monitor (192.168.2.xxx)
[2] COM3 (Serial)
Select upload port [1]:
```
→ Chọn `1` cho OTA upload

**Option B: Specify IP address**

Thêm vào `platformio.ini`:
```ini
[env:esp12e]
upload_protocol = espota
upload_port = 192.168.2.xxx  ; IP của ESP8266
upload_flags = 
  --auth=your_password        ; Nếu có password
```

Hoặc command line:
```bash
platformio run -t upload --upload-port 192.168.2.xxx
```

---

### **Method 2: Arduino IDE**

1. Mở Arduino IDE
2. Tools → Port → Chọn `ESP8266-Monitor at 192.168.2.xxx`
3. Tools → Upload
4. Nhập password (nếu có)

---

### **Method 3: Manual với esptool**

```bash
# Install espota.py
pip install esptool

# Upload
espota.py -i 192.168.2.xxx -p 8266 -f firmware.bin -a your_password
```

---

## 📊 Thông tin kỹ thuật

### **OTA Process:**

1. **Detection**: ESP8266 advertises itself via mDNS
2. **Connection**: Computer connects to ESP8266:8266
3. **Authentication**: Password check (nếu có)
4. **Upload**: Firmware được upload qua WiFi
5. **Flash**: ESP8266 flash firmware mới
6. **Reboot**: Tự động restart với firmware mới

### **Port Usage:**
- **Port 8266**: OTA upload
- **Port 3232**: ArduinoOTA (alternative)
- **mDNS**: `ESP8266-Monitor.local`

### **Memory Layout:**

```
Flash: 4MB
├── [0x00000] Bootloader
├── [0x01000] Firmware (max 1MB)
├── [0x10000] OTA partition (backup)
└── [0x3FB000] SPIFFS/Config
```

---

## 🔍 Troubleshooting

### **Problem: "Device not found"**

**Solution 1: Check network**
```bash
# Ping ESP8266
ping ESP8266-Monitor.local
# hoặc
ping 192.168.2.xxx
```

**Solution 2: Check mDNS**
```bash
# Windows: Install Bonjour
choco install bonjour

# Linux
sudo apt-get install avahi-daemon

# macOS: Built-in
```

**Solution 3: Use IP address**
Thay vì hostname, dùng IP trực tiếp:
```bash
platformio run -t upload --upload-port 192.168.2.xxx
```

---

### **Problem: "Authentication failed"**

**Cause**: Sai password OTA

**Solution**:
1. Check `OTA_PASSWORD` trong `config.h`
2. Rebuild và upload lại qua USB
3. Hoặc update password trong upload command:
   ```bash
   platformio run -t upload --upload-port 192.168.2.xxx --upload-flags="--auth=correct_password"
   ```

---

### **Problem: "Upload timeout"**

**Causes**:
- ESP8266 đang update system data (busy)
- Network congestion
- Firewall blocking port 8266

**Solutions**:
1. Đợi ESP8266 idle (không update data)
2. Disable firewall tạm thời
3. Retry upload

---

### **Problem: "Out of memory"**

**Cause**: Firmware quá lớn (>1MB)

**Solution**:
1. Check firmware size:
   ```
   Flash: [=======   ] 70% (730KB/1044KB)
   ```
2. Nếu >90%, cần optimize:
   - Remove unused libraries
   - Disable features
   - Use PROGMEM more

---

## 🔒 Security Best Practices

### **1. Set Strong Password**
```cpp
#define OTA_PASSWORD "MyVeryStr0ng!P@ssw0rd"
```

### **2. Disable OTA in Production (Optional)**
```cpp
#define OTA_ENABLED false  // Disable sau khi deploy
```

### **3. Use HTTPS/TLS (Advanced)**
```cpp
// Requires ESP8266 >= 2.5.0
ArduinoOTA.setPasswordHash("md5_hash_of_password");
```

### **4. Implement Rollback**
```cpp
// Nếu update fail, tự động rollback về version cũ
ESP.rollbackOTA();
```

---

## 📈 Performance

### **Upload Speed Comparison:**

| Method | Speed | Time (340KB) |
|--------|-------|--------------|
| **USB Serial** | 115200 baud | ~25s |
| **OTA WiFi** | ~50KB/s | ~20s |
| **OTA WiFi (optimized)** | ~100KB/s | ~10s |

### **Network Requirements:**
- **Bandwidth**: Minimum 50KB/s
- **Latency**: <100ms recommended
- **Stability**: No packet loss

---

## 🎛️ Advanced Usage

### **1. Progress Indicator**

Hiện progress trên TFT display:

```cpp
void onOTAProgress(unsigned int progress, unsigned int total) {
  unsigned int percent = (progress * 100) / total;
  
  // Display progress bar
  display.clear();
  display.drawProgressBar(percent);
  display.drawText("Updating: " + String(percent) + "%");
}
```

### **2. Custom Port**

```cpp
ArduinoOTA.setPort(3232);  // Default: 8266
```

### **3. MD5 Verification**

```cpp
ArduinoOTA.setMD5(calculateMD5(firmware));
```

### **4. Conditional OTA**

Chỉ enable OTA khi nhấn button:

```cpp
void loop() {
  if (button.isLongPressed()) {
    ota.setEnabled(true);
  }
  ota.handle();
}
```

---

## 📝 Workflow Recommendations

### **Development:**
```
1. Code changes
2. Upload via USB (fast iteration)
3. Test
4. Repeat
```

### **Testing:**
```
1. Final code
2. Upload via USB
3. Test OTA
4. Upload via OTA
5. Verify
```

### **Production:**
```
1. Deploy via USB (initial)
2. Update via OTA (maintenance)
3. Monitor logs
4. Rollback if needed
```

---

## 🔧 PlatformIO Configuration

Add to `platformio.ini`:

```ini
[env:esp12e]
platform = espressif8266
board = esp12e
framework = arduino

; OTA Configuration
upload_protocol = espota
upload_port = ESP8266-Monitor.local
upload_flags = 
  --port=8266
  --auth=your_password
  
; Alternative: Use IP
; upload_port = 192.168.2.100

; Serial fallback
; upload_protocol = esptool
; upload_port = COM3
```

---

## ✅ Checklist

**Lần đầu setup:**
- [ ] Set `OTA_ENABLED true` in config.h
- [ ] Set hostname và password
- [ ] Upload qua USB
- [ ] Test OTA với dummy update
- [ ] Verify device shows in network

**Mỗi lần update:**
- [ ] Test code locally
- [ ] Build successful
- [ ] ESP8266 connected to WiFi
- [ ] Upload via OTA
- [ ] Verify update successful
- [ ] Test functionality

---

## 📚 References

- [ESP8266 OTA Documentation](https://arduino-esp8266.readthedocs.io/en/latest/ota_updates/readme.html)
- [PlatformIO OTA Guide](https://docs.platformio.org/en/latest/platforms/espressif8266.html#over-the-air-ota-update)
- [ArduinoOTA Library](https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA)

---

## 🎉 Summary

**OTA enables:**
- ✅ Wireless firmware updates
- ✅ Remote maintenance
- ✅ Faster iteration
- ✅ Production-ready deployments

**Memory overhead:**
- RAM: +596 bytes
- Flash: +37KB

**Worth it!** 🚀

---

**Created by**: AnyaChann  
**Date**: 2025-11-05  
**Version**: 2.2.0 - OTA Support
