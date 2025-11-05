# 🎯 WiFiManager Config Portal - Complete Guide

## 🚀 Tính năng mới v3.0

### ✅ What's New:
1. **WiFiManager Integration** - Scan và chọn WiFi từ list
2. **Validation Before Save** - Test WiFi + Server trước khi lưu
3. **Auto Fallback** - Tự động vào config mode nếu lỗi kết nối
4. **Progressive Retry** - Tăng timeout mỗi lần thử reconnect
5. **Smart Detection** - Phân biệt lỗi WiFi vs Server

---

## 📱 Config Portal Workflow

### **Bước 1: Server IP Config (Custom Web)**
```
ESP8266-Config (192.168.4.1)
├── Input: Server IP + Port
├── Validation: IP format check
└── Next: WiFiManager
```

### **Bước 2: WiFi Config (WiFiManager)**
```
ESP8266-Config (Auto-scan WiFi)
├── Scan: Hiển thị list WiFi khả dụng
├── Select: Chọn SSID từ dropdown
├── Input: Password only
└── Connect: Auto test connection
```

### **Bước 3: Validation**
```
System Validation
├── ✓ WiFi Connection Test (15s timeout)
├── ✓ Server Response Test (5s timeout)
└── ✓ Save to EEPROM (if all pass)
```

---

## 🔄 Complete User Journey

### **Lần đầu boot (No config):**

1. **ESP8266 starts in Config Mode**
   ```
   Serial Output:
   No valid config found, starting config portal
   === CONFIG PORTAL WITH WIFIMANAGER ===
   Step 1: Configure Server IP
   AP IP: 192.168.4.1
   ```

2. **TFT Display shows:**
   ```
   CONFIG MODE
   
   Step 1/2:
   Server Config
   
   Connect WiFi:
   ESP8266-Config
   Pass: 12345678
   
   Open: 192.168.4.1
   ```

3. **User connects to ESP8266-Config**
   - From phone/laptop WiFi settings
   - Password: `12345678`

4. **User opens browser → `http://192.168.4.1`**
   ```
   ┌─────────────────────────┐
   │  🔧 ESP8266 Config      │
   ├─────────────────────────┤
   │  Bước 1/2: Server       │
   │                         │
   │  🖥️ Server IP:          │
   │  [192.168.2.60____]     │
   │                         │
   │  🔌 Port:               │
   │  [8080____________]     │
   │                         │
   │  [Tiếp theo: WiFi ➡️]   │
   └─────────────────────────┘
   ```

5. **After submitting server config:**
   ```
   ✅ Bước 1 Hoàn thành!
   
   Server: 192.168.2.60:8080
   
   📱 Bước 2: WiFi Config
   
   ESP8266 sẽ reboot vào
   WiFiManager mode...
   
   Kết nối lại ESP8266-Config
   và chọn WiFi từ list!
   ```

6. **ESP8266 enters WiFiManager mode:**
   ```
   Serial Output:
   Step 2: Configure WiFi with WiFiManager
   Starting WiFiManager portal...
   Entered config mode
   192.168.4.1
   ```

7. **TFT Display shows:**
   ```
   CONFIG MODE
   
   Step 2/2:
   WiFi Selection
   
   Connect WiFi:
   ESP8266-Config
   
   Open: 192.168.4.1
   Select WiFi from list
   ```

8. **User reconnects to ESP8266-Config**
   - Same WiFi: `ESP8266-Config`
   - Same password: `12345678`

9. **User opens browser → Auto-redirects to WiFiManager**
   ```
   ┌─────────────────────────┐
   │  📡 WiFi Configuration  │
   ├─────────────────────────┤
   │  Available Networks:    │
   │                         │
   │  ● Tang 2      (▂▄▆█)   │
   │  ○ Guest WiFi  (▂▄▆_)   │
   │  ○ Office 5G   (▂▄__)   │
   │  ○ ...                  │
   │                         │
   │  🔐 Password:           │
   │  [____________]         │
   │                         │
   │  [Save & Connect 💾]    │
   └─────────────────────────┘
   ```

10. **User selects WiFi and enters password**
    - Click on `Tang 2`
    - Enter password: `66668888`
    - Click `Save`

11. **System validates:**
    ```
    Serial Output:
    
    --- Testing WiFi Connection ---
    SSID: Tang 2
    .....
    ✓ WiFi connected! IP: 192.168.2.123
    
    --- Testing Server Connection ---
    Server: 192.168.2.60:8080
    Testing URL: http://192.168.2.60:8080/system-info
    HTTP Code: 200
    ✓ Server responding!
    
    --- Saving Config to EEPROM ---
    ✓ Config saved successfully!
    
    Rebooting in 3 seconds...
    ```

12. **TFT Display shows:**
    ```
    ✅ SUCCESS!
    
    WiFi: Tang 2
    IP: 192.168.2.123
    
    Server: OK
    192.168.2.60:8080
    
    Saving config...
    Rebooting...
    ```

13. **ESP8266 reboots and loads config:**
    ```
    Serial Output:
    
    --- Loading Config from EEPROM ---
    Magic: 0x4553 (expected: 0x4553)
    ✓ Config loaded successfully!
    
    WiFi connected! IP: 192.168.2.123
    Server responding!
    
    Starting normal operation...
    ```

---

## 🔄 Fallback to Config Mode

### **Auto Fallback Triggers:**

#### **Scenario 1: WiFi Password Changed**
```
Connection attempt 1: Failed (timeout 5s)
Connection attempt 2: Failed (timeout 10s)  
Connection attempt 3: Failed (timeout 15s)
Connection attempt 4: Failed (timeout 20s)
Connection attempt 5: Failed (timeout 25s)

⚠️ Too many connection failures!
Entering config mode for reconfiguration...
```

**TFT Display:**
```
WiFi ERROR!

Failed to connect
after 5 attempts

Possible reasons:
- Wrong password
- Router off
- Out of range

Rebooting to
config mode...
```

#### **Scenario 2: Server Down (Warning Only)**
```
WiFi: Connected ✓
Server: Not responding ✗

⚠️ Warning: Server not responding!
Check if Python server is running at
192.168.2.60:8080

(Will retry in 60 seconds)
(NOT entering config mode)
```

**TFT Display:**
```
WiFi: OK
IP: 192.168.2.123

Server: ERROR
No response from:
192.168.2.60:8080

Check Python
server status
```

---

## 🎯 Key Differences vs Old Version

| Feature | Old (v2.0) | New (v3.0) |
|---------|-----------|-----------|
| **WiFi Selection** | Manual SSID input | Scan + Select from list |
| **Validation** | ❌ None | ✅ Test before save |
| **Wrong Password** | ✗ Saves anyway | ✓ Rejects with error |
| **Server Down** | ✗ Saves anyway | ✓ Rejects with error |
| **Fallback** | ❌ Manual reset | ✅ Auto after 5 fails |
| **Retry Logic** | ❌ Fixed 5s | ✅ Progressive 5-25s |
| **Error Display** | ❌ None | ✅ Detailed on TFT |

---

## 🔧 Configuration Options

### **In `main.cpp`:**

```cpp
// AP credentials
ConfigManager configMgr("ESP8266-Config", "12345678");

// Change to custom:
ConfigManager configMgr("MyDevice", "SecurePass123");
```

### **Fallback thresholds:**

```cpp
// In config_manager.cpp, adjust:

// Fail count before config mode (default: 5)
if (connectionFailCount >= 5) { ... }

// Change to 3 for faster fallback:
if (connectionFailCount >= 3) { ... }

// Reconnect timeout progression
int timeout = 5000 + (connectionFailCount * 5000);
// Try 1: 10s, Try 2: 15s, Try 3: 20s...

// Change to slower:
int timeout = 10000 + (connectionFailCount * 10000);
// Try 1: 20s, Try 2: 30s, Try 3: 40s...
```

### **Server check interval:**

```cpp
// Check server every 60 seconds
if (currentTime - lastConnectionAttempt > 60000) { ... }

// Change to 30 seconds:
if (currentTime - lastConnectionAttempt > 30000) { ... }
```

---

## 🐛 Troubleshooting

### **Problem: WiFiManager page không load**

**Symptoms:**
- Browser không redirect tự động
- Trang trắng hoặc timeout

**Solutions:**
1. Disable mobile data (chỉ dùng WiFi)
2. Manually go to `http://192.168.4.1`
3. Try `http://192.168.4.1:80`
4. Clear browser cache
5. Try different browser (Chrome, Firefox, Safari)

---

### **Problem: WiFi list không hiển thị**

**Symptoms:**
- WiFiManager page load nhưng không có WiFi
- "No networks found"

**Solutions:**
1. Wait 10-15 seconds for scan
2. Click "Scan" button again
3. Check router 2.4GHz enabled (ESP8266 not support 5GHz)
4. Move closer to router

---

### **Problem: "Failed to connect" sau khi enter password**

**Possible Causes & Fixes:**

#### **1. Wrong Password**
```
✗ WiFi connection failed!
Status: 6 (WL_CONNECT_FAILED)
```
→ Double check password (case-sensitive)

#### **2. 5GHz Network**
```
✗ WiFi connection failed!
Status: 1 (WL_NO_SSID_AVAIL)
```
→ ESP8266 only supports 2.4GHz, enable 2.4GHz on router

#### **3. Hidden SSID**
→ WiFiManager may not detect hidden networks
→ Unhide SSID temporarily or use old manual method

#### **4. MAC Filtering**
→ Add ESP8266 MAC to router whitelist
→ MAC: `fc:f5:c4:b2:98:a6` (check Serial Monitor)

---

### **Problem: "Server validation failed"**

**Symptoms:**
```
✓ WiFi connected!
✗ Server validation failed!
Config aborted. Please check server and try again.
```

**Solutions:**
1. **Check Python server running:**
   ```bash
   # On server PC
   curl http://localhost:8080/system-info
   ```

2. **Check IP address:**
   ```bash
   ipconfig  # Windows
   ifconfig  # Linux/Mac
   ```
   Make sure IP matches what you entered

3. **Check firewall:**
   ```bash
   # Windows: Allow port 8080
   # Check "Private networks" in firewall settings
   ```

4. **Test from another device:**
   ```bash
   # From phone browser
   http://192.168.2.60:8080/system-info
   ```

---

### **Problem: Keeps entering config mode after reboot**

**Check Serial Monitor:**

```
--- Loading Config from EEPROM ---
Magic: 0x0000 (expected: 0x4553)
✗ Invalid magic or version
```
→ Config not saved, EEPROM issue → Flash erase and reconfigure

```
--- Loading Config from EEPROM ---
Magic: 0x4553 ✓
Checksum: 0xXX (expected: 0xYY)
✗ Checksum mismatch!
```
→ EEPROM corrupted → Flash erase and reconfigure

```
✓ Config loaded successfully!
WiFi disconnected! Fail count: 1
...
WiFi disconnected! Fail count: 5
⚠️ Too many connection failures!
```
→ WiFi credentials wrong → Will auto-enter config mode

---

## 📊 Memory Usage

```
RAM:   [====      ]  38.3% (31,376 bytes)
Flash: [====      ]  39.8% (415,968 bytes)
```

**vs v2.0:**
- RAM: +1,524 bytes (WiFiManager library)
- Flash: +49,492 bytes (WiFiManager + validation logic)

**Total overhead:** ~50KB Flash acceptable for features gained

---

## 🎉 Summary

**v3.0 Config Portal Features:**

✅ **WiFiManager Integration**
- Scan and display available WiFi networks
- Visual signal strength indicators
- Select from dropdown (no typing SSID)
- Password field only

✅ **Pre-Save Validation**
- Test WiFi connection (15s timeout)
- Test server HTTP response (5s)
- Only save if BOTH pass

✅ **Intelligent Fallback**
- Auto-detect connection failures
- Progressive retry (5s → 25s)
- Auto config mode after 5 fails
- Distinguish WiFi vs Server errors

✅ **Better UX**
- Step-by-step wizard (Server → WiFi)
- Real-time validation feedback
- Detailed error messages on TFT
- Mobile-friendly interface

✅ **Production Ready**
- EEPROM persistence with checksum
- Config verification on boot
- Comprehensive debug logging
- OTA update support

---

**Perfect for:**
- 🏠 Home IoT deployments
- 👨‍👩‍👧 Non-technical users
- 🔧 Field installations
- 🚀 Production devices
- 📱 Mobile configuration

**Next Level:** No more hardcoded credentials! 🎊

---

**Created by**: AnyaChann  
**Date**: 2025-11-05  
**Version**: 3.0.0 - WiFiManager + Validation + Fallback
