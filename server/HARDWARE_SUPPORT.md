# Danh sách phần cứng được hỗ trợ

File này liệt kê các từ khóa dùng để phát hiện phần cứng trong Libre Hardware Monitor.

## CPU

### Intel
- Intel Core (i3, i5, i7, i9)
- Intel Xeon
- Intel Pentium
- Intel Celeron

### AMD
- AMD Ryzen (3, 5, 7, 9)
- AMD EPYC
- AMD Athlon

### Sensor CPU
**Temperatures:**
- "Tctl" (AMD)
- "Tdie" (AMD)
- "Package" (Intel)
- "Core" (fallback)

**Load:**
- "CPU Total"

**Power:**
- "Package"

---

## GPU rời (Discrete GPU)

### NVIDIA
- NVIDIA
- GeForce
- GTX (GTX 1050, 1060, 1070, 1080, 1650, 1660...)
- RTX (RTX 2060, 2070, 2080, 3060, 3070, 3080, 4060, 4070, 4080, 4090...)
- Quadro

### AMD
- AMD Radeon RX (RX 580, 5700 XT, 6800 XT, 7900 XTX...)
- AMD Radeon PRO
- Radeon VII

### Intel
- Intel Arc (A380, A750, A770)
- Intel Iris Xe MAX (GPU rời)

### Sensor GPU rời
**Temperatures:**
- "GPU Core"
- "GPU" (fallback)

**Load:**
- "GPU Core"
- "GPU" (fallback)

**Power:**
- "GPU Package"
- "GPU Power" (fallback)

**Memory:**
- "GPU Memory Used"
- "GPU Memory Total"

---

## iGPU (Integrated GPU)

### AMD
- Radeon Graphics (tích hợp trong CPU Ryzen)
- Radeon(TM) Graphics
- Vega (Ryzen APU)
- RDNA (Ryzen 6000+)

### Intel
- Intel UHD Graphics (thế hệ mới)
- Intel Iris (Iris Plus, Iris Xe)
- Intel HD Graphics (thế hệ cũ)

### Sensor iGPU
**Temperatures:**
- "GPU"
- "Core"
- **Lưu ý:** iGPU thường không có sensor nhiệt độ riêng

**Load:**
- "GPU Core"
- "GPU" (fallback)

---

## RAM

### Phát hiện
- "Memory"
- "RAM"
- "Generic Memory"

### Sensor RAM
**Load:**
- "Memory" (phần trăm sử dụng)

**Data:**
- "Memory Used" (GB)
- "Memory Available" (GB)

---

## Disk/SSD/HDD

### Hãng SSD
- Samsung (Samsung 970 EVO, 980 PRO, 990 PRO...)
- WD (Western Digital - WD Blue, Black, Green, Red...)
- Seagate (Seagate FireCuda, BarraCuda...)
- Toshiba
- Kingston (Kingston A2000, KC3000...)
- Crucial (Crucial MX500, P5 Plus...)
- SanDisk
- Intel (Intel 660p, 670p...)
- Micron
- SK Hynix

### Từ khóa chung
- SSD
- HDD
- NVMe
- M.2

### Sensor Disk
**Temperatures:**
- "Temperature"
- "Drive" (fallback)

**Load:**
- "Used Space" (phần trăm đã sử dụng)

---

## Network

### Loại
- Wi-Fi (Wireless)
- Ethernet (LAN)
- Network (chung)

### Hãng
- Realtek (Realtek PCIe GbE Family Controller...)
- Intel (Intel Wi-Fi 6 AX200, AX201...)
- Qualcomm (Qualcomm Atheros...)
- Broadcom

### Sensor Network
**Throughput:**
- "Download Speed" (KB/s)
- "Upload Speed" (KB/s)

**Lưu ý:** Chỉ lấy interface có traffic (download > 0 hoặc upload > 0)

---

## Cách mở rộng

### Thêm CPU mới
Trong `system_monitor_server.py`, thêm vào điều kiện `is_cpu`:
```python
is_cpu = (
    "Intel Core" in hw_name or 
    "AMD Ryzen" in hw_name or
    "TÊN_CPU_MỚI" in hw_name  # <-- Thêm dòng này
)
```

### Thêm GPU rời mới
Thêm vào list trong điều kiện GPU rời:
```python
elif any(brand in hw_name for brand in ["NVIDIA", "GeForce", "GTX", "RTX",
                                          "TÊN_GPU_MỚI"]):  # <-- Thêm vào đây
```

### Thêm hãng disk mới
Thêm vào list trong điều kiện disk:
```python
elif any(keyword in hw_name.upper() for keyword in ["SAMSUNG", "WD",
                                                      "TÊN_HÃNG_MỚI"]):  # <-- Thêm vào đây
```

---

## Kiểm tra phần cứng được phát hiện

Khi chạy server, sẽ hiển thị:
```
[INFO] Phát hiện X thiết bị phần cứng:
  - AMD Ryzen 7 5800H with Radeon Graphics
  - Generic Memory
  - NVIDIA GeForce RTX 3060 Laptop GPU
  - AMD Radeon(TM) Graphics
  - SAMSUNG MZVLQ512HBLU-00BH1
  - SAMSUNG MZVL21T0HCLR-00B00
  - Qualcomm QCA61x4A Wireless Network Adapter

[THỐNG KÊ]
  CPU: ✓
  RAM: ✓
  GPU rời: ✓
  iGPU: ✓
  Disk: 2 thiết bị
  Network: ✓
```

Nếu thiết bị không được phát hiện, kiểm tra tên chính xác trong Libre Hardware Monitor và thêm vào điều kiện tương ứng.
