"""
System Monitor Server - Lấy thông tin từ Libre Hardware Monitor và cung cấp HTTP API
Yêu cầu: 
- Libre Hardware Monitor đang chạy với Remote Web Server enabled
- Python 3.7+
- pip install flask requests
"""

from flask import Flask, jsonify
import requests
import socket

app = Flask(__name__)

# Libre Hardware Monitor server URL
LIBRE_HW_MONITOR_URL = "http://192.168.2.60:8085/data.json"

def parse_value(value_str):
    """Parse giá trị từ string, loại bỏ đơn vị"""
    if not value_str:
        return 0.0
    try:
        # Loại bỏ đơn vị (bao gồm cả ký tự Unicode như Â°C) và chuyển dấu phẩy thành dấu chấm
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

def get_system_info():
    """Lấy thông tin hệ thống từ Libre Hardware Monitor"""
    try:
        response = requests.get(LIBRE_HW_MONITOR_URL, timeout=5)
        data = response.json()
        
        result = {
            "cpu": {"name": "", "temp": 0, "load": 0, "power": 0},
            "ram": {"used": 0, "total": 0, "percent": 0},
            "gpu_discrete": {"name": "", "temp": 0, "load": 0, "power": 0, "mem_used": 0, "mem_total": 0},
            "gpu_integrated": {"name": "", "temp": 0, "load": 0},
            "disk": [],
            "network": {"name": "", "upload": 0, "download": 0}
        }
        
        # Duyệt qua tất cả hardware
        # Cấu trúc: root -> Children[0] (Computer) -> Children[] (các thiết bị)
        root_children = data.get("Children", [])
        if not root_children:
            print("[ERROR] No root children found!")
            return result
        
        computer_node = root_children[0]  # LAPTOP-CTER
        hardware_list = computer_node.get("Children", [])
        print(f"\n[INFO] Phát hiện {len(hardware_list)} thiết bị phần cứng:")
        
        
        detected_hardware = {"cpu": False, "ram": False, "gpu_discrete": False, 
                            "gpu_integrated": False, "disk": 0, "network": False}
        
        for hw in hardware_list:
            print(f"  - {hw.get('Text', 'Unknown')}")
            hw_name = hw.get("Text", "")
            sensors = hw.get("Children", [])
            hw_type = hw.get("ImageURL", "").lower()  # Sử dụng ImageURL để xác định loại phần cứng
            
            # CPU - Phát hiện linh hoạt (Intel, AMD, hoặc các hãng khác)
            is_cpu = (
                "Intel Core" in hw_name or 
                "AMD Ryzen" in hw_name or 
                "Intel Xeon" in hw_name or
                "AMD EPYC" in hw_name or
                "Intel Pentium" in hw_name or
                "Intel Celeron" in hw_name or
                "AMD Athlon" in hw_name or
                "cpu.png" in hw_type
            )
            
            if is_cpu:
                detected_hardware["cpu"] = True
                result["cpu"]["name"] = hw_name
                
                # Lấy nhiệt độ CPU
                for sensor_group in sensors:
                    if sensor_group.get("Text") == "Temperatures":
                        for temp_sensor in sensor_group.get("Children", []):
                            temp_name = temp_sensor.get("Text", "")
                            if "Tctl" in temp_name or "Package" in temp_name or "Core" in temp_name:
                                result["cpu"]["temp"] = parse_value(temp_sensor.get("Value", ""))
                                break
                        if result["cpu"]["temp"] > 0:
                            break
                
                # Lấy CPU Load
                for sensor_group in sensors:
                    if sensor_group.get("Text") == "Load":
                        for load_sensor in sensor_group.get("Children", []):
                            if "CPU Total" in load_sensor.get("Text", ""):
                                result["cpu"]["load"] = parse_value(load_sensor.get("Value", ""))
                                break
                
                # Lấy CPU Power
                for sensor_group in sensors:
                    if sensor_group.get("Text") == "Powers":
                        for power_sensor in sensor_group.get("Children", []):
                            if "Package" in power_sensor.get("Text", ""):
                                result["cpu"]["power"] = parse_value(power_sensor.get("Value", ""))
                                break
            
            # RAM - Phát hiện linh hoạt
            elif any(keyword in hw_name for keyword in ["Memory", "RAM", "Generic Memory"]):
                detected_hardware["ram"] = True
                result["ram"]["percent"] = find_sensor(sensors, "Load", "Memory")
                result["ram"]["used"] = find_sensor(sensors, "Data", "Memory Used")
                result["ram"]["total"] = result["ram"]["used"] + find_sensor(sensors, "Data", "Memory Available")
            
            # GPU rời (Discrete GPU) - NVIDIA, AMD, Intel Arc
            elif any(brand in hw_name for brand in ["NVIDIA", "GeForce", "GTX", "RTX", "Quadro", 
                                                      "AMD Radeon RX", "AMD Radeon PRO", "Radeon VII",
                                                      "Intel Arc", "Intel Iris Xe MAX"]):
                detected_hardware["gpu_discrete"] = True
                result["gpu_discrete"]["name"] = hw_name
                result["gpu_discrete"]["temp"] = find_sensor(sensors, "Temperatures", "GPU Core")
                if result["gpu_discrete"]["temp"] == 0:
                    result["gpu_discrete"]["temp"] = find_sensor(sensors, "Temperatures", "GPU")
                result["gpu_discrete"]["load"] = find_sensor(sensors, "Load", "GPU Core")
                if result["gpu_discrete"]["load"] == 0:
                    result["gpu_discrete"]["load"] = find_sensor(sensors, "Load", "GPU")
                result["gpu_discrete"]["power"] = find_sensor(sensors, "Powers", "GPU Package")
                if result["gpu_discrete"]["power"] == 0:
                    result["gpu_discrete"]["power"] = find_sensor(sensors, "Powers", "GPU Power")
                result["gpu_discrete"]["mem_used"] = int(find_sensor(sensors, "Data", "GPU Memory Used"))
                result["gpu_discrete"]["mem_total"] = int(find_sensor(sensors, "Data", "GPU Memory Total"))
            
            # iGPU (Integrated GPU) - AMD Radeon Graphics, Intel UHD/Iris
            elif any(keyword in hw_name for keyword in ["Radeon Graphics", "Radeon(TM) Graphics",
                                                         "Intel UHD", "Intel Iris", "Intel HD Graphics",
                                                         "Vega", "RDNA"]):
                detected_hardware["gpu_integrated"] = True
                result["gpu_integrated"]["name"] = hw_name
                # iGPU thường không có sensor nhiệt độ riêng
                for sensor_group in sensors:
                    if sensor_group.get("Text") == "Temperatures":
                        for temp_sensor in sensor_group.get("Children", []):
                            temp_name = temp_sensor.get("Text", "")
                            if "GPU" in temp_name or "Core" in temp_name:
                                result["gpu_integrated"]["temp"] = parse_value(temp_sensor.get("Value", ""))
                                break
                        break
                result["gpu_integrated"]["load"] = find_sensor(sensors, "Load", "GPU Core")
                if result["gpu_integrated"]["load"] == 0:
                    result["gpu_integrated"]["load"] = find_sensor(sensors, "Load", "GPU")
            
            # Disk/SSD/HDD - Phát hiện linh hoạt tất cả hãng
            elif any(keyword in hw_name.upper() for keyword in ["SAMSUNG", "WD", "SEAGATE", "TOSHIBA", 
                                                                  "KINGSTON", "CRUCIAL", "SANDISK", 
                                                                  "INTEL", "MICRON", "HYNIX",
                                                                  "SSD", "HDD", "NVME", "M.2"]) or \
                 "storage.png" in hw_type or "hdd.png" in hw_type:
                # Tìm nhiệt độ với nhiều tên sensor khác nhau
                temp = find_sensor(sensors, "Temperatures", "Temperature")
                if temp == 0:
                    temp = find_sensor(sensors, "Temperatures", "Drive")
                
                detected_hardware["disk"] += 1
                disk_info = {
                    "name": hw_name[:30],  # Giới hạn độ dài tên
                    "temp": temp,
                    "load": find_sensor(sensors, "Load", "Used Space")
                }
                result["disk"].append(disk_info)
            
            # Network - Phát hiện linh hoạt (Wi-Fi, Ethernet, các card mạng khác)
            elif any(keyword in hw_name for keyword in ["Wi-Fi", "Ethernet", "Network", 
                                                         "Wireless", "LAN", "Realtek",
                                                         "Intel", "Qualcomm", "Broadcom"]) or \
                 "nic.png" in hw_type:
                download = find_sensor(sensors, "Throughput", "Download Speed")
                upload = find_sensor(sensors, "Throughput", "Upload Speed")
                # Chỉ lấy interface có traffic
                if download > 0 or upload > 0:
                    detected_hardware["network"] = True
                    result["network"]["name"] = hw_name[:30]  # Giới hạn độ dài
                    result["network"]["download"] = download
                    result["network"]["upload"] = upload
        
        # Giới hạn số disk tối đa 2 (để tiết kiệm dung lượng JSON)
        result["disk"] = result["disk"][:2]
        
        # In thống kê phần cứng phát hiện được
        print("\n[THỐNG KÊ]")
        print(f"  CPU: {'✓' if detected_hardware['cpu'] else '✗'}")
        print(f"  RAM: {'✓' if detected_hardware['ram'] else '✗'}")
        print(f"  GPU rời: {'✓' if detected_hardware['gpu_discrete'] else '✗'}")
        print(f"  iGPU: {'✓' if detected_hardware['gpu_integrated'] else '✗'}")
        print(f"  Disk: {detected_hardware['disk']} thiết bị")
        print(f"  Network: {'✓' if detected_hardware['network'] else '✗'}\n")
        
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
    return """
    <h1>System Monitor Server</h1>
    <p>Server đang chạy!</p>
    <p>API endpoint: <a href="/system-info">/system-info</a></p>
    """

def get_local_ip():
    """Lấy địa chỉ IP local"""
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return "127.0.0.1"

if __name__ == '__main__':
    local_ip = get_local_ip()
    print("="*50)
    print(f"Server đang chạy tại: http://{local_ip}:8080")
    print(f"API endpoint: http://{local_ip}:8080/system-info")
    print("="*50)
    print("\nĐảm bảo Libre Hardware Monitor đang chạy!")
    print("Cấu hình ESP8266:")
    print(f'  const char* serverUrl = "http://{local_ip}:8080/system-info";')
    print("="*50)
    
    app.run(host='0.0.0.0', port=8080, debug=False)
