/*
 * OTA Manager Implementation
 */

#include "config.h"  // Must be first for TFT definitions
#include "ota_manager.h"
#include "display_manager.h"

OTAManager::OTAManager(const char* deviceName, const char* otaPassword)
  : hostname(deviceName), password(otaPassword), enabled(true), display(nullptr) {}

void OTAManager::begin() {
  if (!enabled) return;
  
  // Set hostname
  ArduinoOTA.setHostname(hostname.c_str());
  
  // Set password (nếu có)
  if (password.length() > 0) {
    ArduinoOTA.setPassword(password.c_str());
  }
  
  // Callbacks
  ArduinoOTA.onStart([this]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "firmware" : "filesystem";
    
    Serial.println(F("\n========== OTA UPDATE START =========="));
    Serial.print(F("Type: "));
    Serial.println(type);
    
    if (display) {
      display->clear();
      display->drawText(20, 30, "OTA UPDATE", ST77XX_YELLOW, 2);
      display->drawText(15, 60, "Updating...", ST77XX_WHITE, 1);
      display->drawText(10, 80, type.c_str(), ST77XX_CYAN, 1);
    }
  });
  
  ArduinoOTA.onEnd([this]() {
    Serial.println(F("\n========== OTA COMPLETE =========="));
    
    if (display) {
      display->clear();
      display->drawText(25, 50, "UPDATE", ST77XX_GREEN, 2);
      display->drawText(20, 80, "COMPLETE!", ST77XX_GREEN, 2);
      display->drawText(15, 115, "Restarting...", ST77XX_WHITE, 1);
    }
    delay(2000);
  });
  
  ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
    static unsigned int lastPercent = 0;
    unsigned int percent = (progress * 100) / total;
    
    // Update every 5%
    if (percent != lastPercent && percent % 5 == 0) {
      Serial.printf("Progress: %u%%\r", percent);
      
      if (display) {
        // Clear progress area
        display->drawText(10, 100, "                ", ST77XX_BLACK, 2);
        
        // Draw progress bar
        int barWidth = 108;  // 128 - 20 margin
        int barHeight = 20;
        int barX = 10;
        int barY = 100;
        int fillWidth = (barWidth * percent) / 100;
        
        // Border
        for (int i = 0; i < barHeight; i++) {
          for (int j = 0; j < barWidth; j++) {
            uint16_t color = ST77XX_BLACK;
            if (i == 0 || i == barHeight-1 || j == 0 || j == barWidth-1) {
              color = ST77XX_WHITE;  // Border
            } else if (j < fillWidth) {
              color = ST77XX_GREEN;  // Fill
            }
            // Simple pixel draw (can optimize with fillRect later)
          }
        }
        
        // Draw percentage text
        char percentText[8];
        snprintf(percentText, sizeof(percentText), "%u%%", percent);
        display->drawText(55, 105, percentText, ST77XX_YELLOW, 2);
      }
      
      lastPercent = percent;
    }
  });
  
  ArduinoOTA.onError([this](ota_error_t error) {
    Serial.printf("\n========== OTA ERROR[%u] ==========\n", error);
    const char* errorMsg = "Unknown Error";
    
    if (error == OTA_AUTH_ERROR) errorMsg = "Auth Failed";
    else if (error == OTA_BEGIN_ERROR) errorMsg = "Begin Failed";
    else if (error == OTA_CONNECT_ERROR) errorMsg = "Connect Failed";
    else if (error == OTA_RECEIVE_ERROR) errorMsg = "Receive Failed";
    else if (error == OTA_END_ERROR) errorMsg = "End Failed";
    
    Serial.println(errorMsg);
    
    if (display) {
      display->clear();
      display->drawText(15, 40, "OTA ERROR!", ST77XX_RED, 2);
      display->drawText(10, 75, errorMsg, ST77XX_YELLOW, 1);
      display->drawText(15, 110, "Please retry", ST77XX_WHITE, 1);
    }
    delay(3000);
  });
  
  ArduinoOTA.begin();
  
  Serial.println(F("========== OTA READY =========="));
  Serial.print(F("Hostname: "));
  Serial.println(hostname);
  Serial.print(F("Password: "));
  Serial.println(password.length() > 0 ? "Set (protected)" : "None (open)");
  Serial.println(F("=================================="));
}

void OTAManager::handle() {
  if (enabled) {
    ArduinoOTA.handle();
  }
}

void OTAManager::setEnabled(bool enable) {
  enabled = enable;
}

bool OTAManager::isEnabled() {
  return enabled;
}

void OTAManager::setDisplayManager(DisplayManager* dm) {
  display = dm;
}
