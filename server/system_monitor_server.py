"""
System Monitor Server - Lấy thông tin từ Libre Hardware Monitor và cung cấp HTTP API
Yêu cầu: 
- Libre Hardware Monitor đang chạy với Remote Web Server enabled
- Python 3.7+
- pip install flask requests python-dotenv
"""

from flask import Flask, jsonify
import requests
import socket
import os
from dotenv import load_dotenv

# Load cấu hình từ .env ở folder server
load_dotenv()

app = Flask(__name__)

def get_local_ip():
    """GET local IP address of the machine"""
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return "127.0.0.1"

# Cấu hình từ .env
DEBUG_MODE = os.getenv('DEBUG_MODE', 'false').lower() == 'true'
SERVER_PORT = int(os.getenv('SERVER_PORT', '8080'))
LIBRE_HW_MONITOR_PORT = int(os.getenv('LIBRE_HW_MONITOR_PORT', '8085'))
MAX_DISKS = int(os.getenv('MAX_DISKS', '2'))
PC_IP_ADDRESS = os.getenv('PC_IP_ADDRESS', '').strip()

# Nếu không có IP trong .env, tự động phát hiện
if not PC_IP_ADDRESS:
    PC_IP_ADDRESS = get_local_ip()

# Libre Hardware Monitor server URL
LIBRE_HW_MONITOR_URL = f"http://{PC_IP_ADDRESS}:{LIBRE_HW_MONITOR_PORT}/data.json"

# Hardware detection patterns
CPU_KEYWORDS = ("Intel Core", "AMD Ryzen", "Intel Xeon", "AMD EPYC", "Intel Pentium", "Intel Celeron", "AMD Athlon")
RAM_KEYWORDS = ("Memory", "RAM", "Generic Memory")
GPU_DISCRETE_KEYWORDS = ("NVIDIA", "GeForce", "GTX", "RTX", "Quadro", "AMD Radeon RX", "AMD Radeon PRO", "Radeon VII", "Intel Arc", "Intel Iris Xe MAX")
GPU_INTEGRATED_KEYWORDS = ("Radeon Graphics", "Radeon(TM) Graphics", "Intel UHD", "Intel Iris", "Intel HD Graphics", "Vega", "RDNA")
DISK_KEYWORDS = ("SAMSUNG", "WD", "SEAGATE", "TOSHIBA", "KINGSTON", "CRUCIAL", "SANDISK", "INTEL", "MICRON", "HYNIX", "SSD", "HDD", "NVME", "M.2")
NETWORK_KEYWORDS = ("Wi-Fi", "Ethernet", "Network", "Wireless", "LAN", "Realtek", "Intel", "Qualcomm", "Broadcom")

def debug_print(message):
    """In log chỉ khi DEBUG_MODE = true"""
    if DEBUG_MODE:
        print(message)

def parse_value(value_str):
    """Parse giá trị từ string, loại bỏ đơn vị"""
    if not value_str:
        return 0.0
    try:
        cleaned = str(value_str).split()[0].replace(",", ".")
        return float(cleaned)
    except (ValueError, IndexError):
        return 0.0

def find_sensor(sensors, sensor_type, keyword):
    """Tìm sensor theo type và keyword"""
    for sensor in sensors:
        if sensor.get("Text") == sensor_type:
            for item in sensor.get("Children", []):
                if keyword in item.get("Text", ""):
                    return parse_value(item.get("Value", "0"))
    return 0.0

def find_sensor_multi(sensors, sensor_type, keywords):
    """Tìm sensor với nhiều keywords (fallback)"""
    for keyword in keywords:
        value = find_sensor(sensors, sensor_type, keyword)
        if value > 0:
            return value
    return 0.0

def get_system_info():
    """Get SYSTEM Statistics from Libre Hardware Monitor"""
    try:
        response = requests.get(LIBRE_HW_MONITOR_URL, timeout=5)
        data = response.json()
        
        # Khởi tạo result với thứ tự cố định
        from collections import OrderedDict
        result = OrderedDict([
            ("cpu", {"name": "", "temp": 0, "load": 0, "power": 0}),
            ("ram", {"used": 0, "total": 0, "percent": 0}),
            ("gpu_discrete", {"name": "", "temp": 0, "load": 0, "power": 0, "mem_used": 0, "mem_total": 0}),
            ("gpu_integrated", {"name": "", "temp": 0, "load": 0}),
            ("disk", []),
            ("network", {"name": "", "upload": 0, "download": 0})
        ])
        
        # Duyệt qua tất cả hardware
        # Cấu trúc: root -> Children[0] (Computer) -> Children[] (các thiết bị)
        root_children = data.get("Children", [])
        if not root_children:
            print("[ERROR] No root children found!")
            return result
        
        computer_node = root_children[0]  # LAPTOP-CTER
        hardware_list = computer_node.get("Children", [])
        debug_print(f"\n[INFO] Detected {len(hardware_list)} hardware devices:")
        
        
        detected_hardware = {"cpu": False, "ram": False, "gpu_discrete": False, 
                            "gpu_integrated": False, "disk": 0, "network": False}
        
        for hw in hardware_list:
            debug_print(f"  - {hw.get('Text', 'Unknown')}")
            hw_name = hw.get("Text", "")
            sensors = hw.get("Children", [])
            hw_type = hw.get("ImageURL", "").lower()  # Sử dụng ImageURL để xác định loại phần cứng
            
            # CPU - Phát hiện linh hoạt
            if any(kw in hw_name for kw in CPU_KEYWORDS) or "cpu.png" in hw_type:
                detected_hardware["cpu"] = True
                result["cpu"]["name"] = hw_name
                result["cpu"]["temp"] = find_sensor_multi(sensors, "Temperatures", ("Tctl", "Package", "Core"))
                result["cpu"]["load"] = find_sensor(sensors, "Load", "CPU Total")
                result["cpu"]["power"] = find_sensor(sensors, "Powers", "Package")
            
            # RAM - Phát hiện linh hoạt
            elif any(kw in hw_name for kw in RAM_KEYWORDS):
                detected_hardware["ram"] = True
                result["ram"]["percent"] = find_sensor(sensors, "Load", "Memory")
                result["ram"]["used"] = find_sensor(sensors, "Data", "Memory Used")
                result["ram"]["total"] = result["ram"]["used"] + find_sensor(sensors, "Data", "Memory Available")
            
            # GPU rời (Discrete GPU) - NVIDIA, AMD, Intel Arc
            elif any(kw in hw_name for kw in GPU_DISCRETE_KEYWORDS):
                detected_hardware["gpu_discrete"] = True
                result["gpu_discrete"]["name"] = hw_name
                result["gpu_discrete"]["temp"] = find_sensor_multi(sensors, "Temperatures", ("GPU Core", "GPU"))
                result["gpu_discrete"]["load"] = find_sensor_multi(sensors, "Load", ("GPU Core", "GPU"))
                result["gpu_discrete"]["power"] = find_sensor_multi(sensors, "Powers", ("GPU Package", "GPU Power"))
                result["gpu_discrete"]["mem_used"] = int(find_sensor(sensors, "Data", "GPU Memory Used"))
                result["gpu_discrete"]["mem_total"] = int(find_sensor(sensors, "Data", "GPU Memory Total"))
            
            # iGPU (Integrated GPU) - AMD Radeon Graphics, Intel UHD/Iris
            elif any(kw in hw_name for kw in GPU_INTEGRATED_KEYWORDS):
                detected_hardware["gpu_integrated"] = True
                result["gpu_integrated"]["name"] = hw_name
                result["gpu_integrated"]["temp"] = find_sensor_multi(sensors, "Temperatures", ("GPU", "Core"))
                result["gpu_integrated"]["load"] = find_sensor_multi(sensors, "Load", ("GPU Core", "GPU"))
            
            # Disk/SSD/HDD - Phát hiện linh hoạt
            elif any(kw in hw_name.upper() for kw in DISK_KEYWORDS) or \
                 "storage.png" in hw_type or "hdd.png" in hw_type:
                detected_hardware["disk"] += 1
                result["disk"].append({
                    "name": hw_name[:30],
                    "temp": find_sensor_multi(sensors, "Temperatures", ("Temperature", "Drive")),
                    "load": find_sensor(sensors, "Load", "Used Space")
                })
            
            # Network - Phát hiện linh hoạt
            elif any(kw in hw_name for kw in NETWORK_KEYWORDS) or "nic.png" in hw_type:
                download = find_sensor(sensors, "Throughput", "Download Speed")
                upload = find_sensor(sensors, "Throughput", "Upload Speed")
                if download > 0 or upload > 0:
                    detected_hardware["network"] = True
                    result["network"]["name"] = hw_name[:30]
                    result["network"]["download"] = download
                    result["network"]["upload"] = upload
        
        # Giới hạn số disk (cấu hình trong .env)
        result["disk"] = result["disk"][:MAX_DISKS]
        
        # In thống kê phần cứng phát hiện được
        if DEBUG_MODE:
            stats = [
                f"CPU: {'✓' if detected_hardware['cpu'] else '✗'}",
                f"RAM: {'✓' if detected_hardware['ram'] else '✗'}",
                f"GPU rời: {'✓' if detected_hardware['gpu_discrete'] else '✗'}",
                f"iGPU: {'✓' if detected_hardware['gpu_integrated'] else '✗'}",
                f"Disk: {detected_hardware['disk']} thiết bị",
                f"Network: {'✓' if detected_hardware['network'] else '✗'}"
            ]
            print("\n[Statistics]\n  " + "\n  ".join(stats) + "\n")
        
        return result
    
    except requests.exceptions.RequestException as e:
        print(f"Lỗi kết nối: {str(e)}")
        return {"error": str(e), "message": "Không thể kết nối đến Libre Hardware Monitor!"}
    except Exception as e:
        print(f"Lỗi xử lý: {str(e)}")
        return {"error": str(e), "message": "Lỗi khi xử lý dữ liệu!"}

@app.route('/system-info', methods=['GET'])
def system_info():
    """API endpoint trả về thông tin hệ thống"""
    data = get_system_info()
    return jsonify(data)

@app.route('/', methods=['GET'])
def home():
    """Trang chủ"""
    return f"""
    <h1>System Monitor Server</h1>
    <p>Server is running!</p>
    <p>Server IP: <strong>{PC_IP_ADDRESS}:{SERVER_PORT}</strong></p>
    <p>API endpoint: <a href="/system-info">/system-info</a></p>
    <p>Libre HW Monitor: <a href="http://{PC_IP_ADDRESS}:{LIBRE_HW_MONITOR_PORT}" target="_blank">
       http://{PC_IP_ADDRESS}:{LIBRE_HW_MONITOR_PORT}</a></p>
    """

if __name__ == '__main__':
    print("="*50)
    print("System Monitor Server v1.0")
    print("="*50)
    print(f"Server: http://{PC_IP_ADDRESS}:{SERVER_PORT}")
    print(f"API: http://{PC_IP_ADDRESS}:{SERVER_PORT}/system-info")
    print(f"Libre HW Monitor: http://{PC_IP_ADDRESS}:{LIBRE_HW_MONITOR_PORT}")
    print(f"Debug Mode: {'ON' if DEBUG_MODE else 'OFF'}")
    print(f"Max Disks: {MAX_DISKS}")
    print("="*50)
    print("\nĐảm bảo Libre Hardware Monitor đang chạy!")
    print("Cấu hình ESP8266:")
    print(f'  SERVER_IP = "{PC_IP_ADDRESS}"\n  SERVER_PORT = "{SERVER_PORT}"')
    print("="*50)
    print("\nTip: Chỉnh sửa server/.env để thay đổi cấu hình\n")
    
    app.run(host='0.0.0.0', port=SERVER_PORT, debug=False)
