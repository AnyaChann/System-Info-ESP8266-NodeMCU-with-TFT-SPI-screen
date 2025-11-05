# Changelog

All notable changes to this project will be documented in this file.

## [1.2.0] - 2025-11-05

### Added ✨
- **Centralized Configuration**: File .env ở thư mục gốc
  - `PC_IP_ADDRESS`: IP của PC (tự động phát hiện nếu để trống)
- **Config Template**: `include/config.h.example`
  - Copy thành `config.h` để config ESP8266
  - Không cần script phức tạp

### Changed 🔄
- `.env` di chuyển từ `server/.env` → `.env` (root)
- ESP8266 load config từ `config.h` (copy từ `config.h.example`)
- Python server tự động load .env từ thư mục gốc
- Workflow đơn giản: Copy config → Chỉnh → Upload

### Removed 🗑️
- Xóa `generate_config.py` (không cần nữa)
- Xóa các file .md dư thừa (giữ README, QUICKSTART, CHANGELOG)

## [1.1.0] - 2025-11-05

### Added ✨
- **Environment Configuration (.env)**: Cấu hình server qua file .env
  - `DEBUG_MODE`: Bật/tắt debug logging
  - `SERVER_PORT`: Tùy chỉnh port server
  - `LIBRE_HW_MONITOR_PORT`: Tùy chỉnh port Libre Hardware Monitor
  - `MAX_DISKS`: Giới hạn số disk hiển thị
- **Debug Mode**: Hiển thị log chi tiết khi cần
  - Danh sách phần cứng phát hiện được
  - Thống kê thiết bị (CPU, RAM, GPU, Disk, Network)
- **Documentation**: Tài liệu đầy đủ
  - `README.md`: Hướng dẫn chi tiết
  - `QUICKSTART.md`: Hướng dẫn nhanh 5 phút
  - `DEBUG.md`: Hướng dẫn debug và khắc phục sự cố
  - `HARDWARE_SUPPORT.md`: Danh sách phần cứng được hỗ trợ

### Changed 🔄
- **Hardware Detection**: Cải thiện phát hiện phần cứng linh hoạt
  - Hỗ trợ nhiều hãng CPU (Intel, AMD)
  - Hỗ trợ GPU rời: NVIDIA, AMD, Intel Arc
  - Hỗ trợ iGPU: AMD Radeon Graphics, Intel UHD/Iris/HD
  - Hỗ trợ nhiều hãng SSD/HDD
  - Hỗ trợ nhiều loại network adapter
- **API Response**: Đổi tên trường để rõ nghĩa hơn
  - `gpu_nvidia` → `gpu_discrete` (GPU rời)
  - `gpu_amd` → `gpu_integrated` (iGPU)
- **Logging**: Cải thiện output server
  - Hiển thị phiên bản
  - Hiển thị cấu hình đang dùng
  - Tips sử dụng

### Fixed 🐛
- AMD iGPU temperature hiển thị "N/A" thay vì "0.0°C" khi không có sensor
- GPU rời hiển thị Power và VRAM chỉ khi có giá trị

## [1.0.0] - 2025-11-04

### Added ✨
- **Initial Release**: ESP8266 System Monitor
- **Core Features**:
  - Lấy thông tin từ Libre Hardware Monitor qua HTTP API
  - Python Flask server làm intermediary
  - ESP8266 firmware hiển thị qua Serial Monitor
- **Data Monitoring**:
  - CPU: Tên, nhiệt độ, load, power
  - RAM: Used, total, percent
  - GPU (NVIDIA): Tên, nhiệt độ, load, power, VRAM
  - GPU (AMD iGPU): Tên, load
  - Disk/SSD: Tên, nhiệt độ, used space
  - Network: Tên, upload/download speed
- **Optimizations**:
  - JSON size tối ưu cho ESP8266 (< 2KB)
  - DynamicJsonDocument với 2048 bytes buffer
  - Giới hạn 2 disk, chỉ active network

### Technical Details 🔧
- **ESP8266**: Arduino framework, ArduinoJson v6.21.3
- **Python Server**: Flask, requests
- **Build Tool**: PlatformIO
- **Data Source**: Libre Hardware Monitor Remote Web Server

---

## Legend

- ✨ Added: Tính năng mới
- 🔄 Changed: Thay đổi tính năng hiện có
- 🐛 Fixed: Sửa lỗi
- 🔧 Technical: Chi tiết kỹ thuật
- 🗑️ Removed: Xóa tính năng
- ⚠️ Deprecated: Tính năng sẽ bị xóa
- 🔒 Security: Bảo mật
