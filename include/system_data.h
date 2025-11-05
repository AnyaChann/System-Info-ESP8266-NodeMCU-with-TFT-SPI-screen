/*
 * System Data Structures
 * Định nghĩa các struct và kiểu dữ liệu cho system monitor
 */

#ifndef SYSTEM_DATA_H
#define SYSTEM_DATA_H

#include <Arduino.h>

// System data struct
struct SystemData {
  String cpuName;
  float cpuTemp, cpuLoad, cpuPower;
  float ramUsed, ramTotal, ramPercent;
  String gpuName;
  float gpuTemp, gpuLoad, gpuPower;
  int gpuMemUsed, gpuMemTotal;
  String disk1Name, disk2Name;
  float disk1Temp, disk1Load, disk2Temp, disk2Load;
  String netName;
  float netDown, netUp;
  bool hasData;
  
  // Constructor
  SystemData() : 
    cpuTemp(0), cpuLoad(0), cpuPower(0),
    ramUsed(0), ramTotal(0), ramPercent(0),
    gpuTemp(0), gpuLoad(0), gpuPower(0),
    gpuMemUsed(0), gpuMemTotal(0),
    disk1Temp(0), disk1Load(0), disk2Temp(0), disk2Load(0),
    netDown(0), netUp(0),
    hasData(false) {}
};

// Color definitions (RGB565)
#define COLOR_BG       0x0000  // Black
#define COLOR_TEXT     0xFFFF  // White
#define COLOR_HEADER   0x07FF  // Cyan
#define COLOR_CPU      0xF800  // Red
#define COLOR_RAM      0x07E0  // Green
#define COLOR_GPU      0xFFE0  // Yellow
#define COLOR_DISK     0xF81F  // Magenta
#define COLOR_NET      0x001F  // Blue

#endif // SYSTEM_DATA_H
