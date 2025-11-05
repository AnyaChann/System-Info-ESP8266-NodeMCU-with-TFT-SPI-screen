# 🚀 Quick Start - OTA Update

## 🎯 TL;DR

**Update firmware qua WiFi, không cần cắm USB!**

---

## 📋 Setup (1 lần duy nhất)

### **1. Enable OTA trong `config.h`:**

```cpp
#define OTA_ENABLED true
#define OTA_HOSTNAME "ESP8266-Monitor"
#define OTA_PASSWORD "your_password"  // Optional nhưng recommended
```

### **2. Upload lần đầu qua USB:**

```bash
platformio run --target upload --environment esp12e
```

⚠️ **Chỉ lần đầu cần USB!**

---

## 🔄 Update lần sau (qua WiFi)

### **Method 1: Auto-detect (Easiest)**

```bash
platformio run --target upload
```

Chọn `ESP8266-Monitor` khi hỏi!

### **Method 2: Specify IP**

```bash
platformio run --target upload --upload-port 192.168.2.xxx
```

Replace `xxx` bằng IP của ESP8266.

---

## ❓ Làm sao biết IP của ESP8266?

### **Option 1: Xem trên TFT display**
→ Góc dưới màn hình hiển thị: `WiFi: 192.168.2.xxx`

### **Option 2: Xem Serial Monitor**
```bash
platformio device monitor
```
→ Boot log sẽ in ra IP

### **Option 3: Router admin page**
→ Check DHCP clients list

### **Option 4: Network scan**
```bash
# Windows
arp -a | findstr fc-f5-c4

# Linux/macOS
arp -a | grep fc:f5:c4
```

---

## 🎊 Done!

Từ giờ mọi update chỉ cần:

```bash
platformio run -t upload --upload-port <IP_ESP8266>
```

**No more USB cables!** 🎉

---

## 🆘 Troubleshooting

### Device not found?
1. Check ESP8266 đang bật
2. Check cùng WiFi network
3. Ping thử: `ping 192.168.2.xxx`
4. Use IP thay vì hostname

### Authentication failed?
→ Check `OTA_PASSWORD` đúng trong config.h

### Still not working?
→ Xem chi tiết trong `OTA_GUIDE.md`

---

**Happy coding!** 🚀
