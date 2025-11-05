/*
 * Config Portal HTML Implementation
 */

#include "config_portal.h"

String ConfigPortal::generateServerConfigHTML() {
  String html = F("<!DOCTYPE html><html><head><meta charset='UTF-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>Server Config</title>");
  html += F("<style>");
  html += F("body{font-family:Arial;margin:20px;background:#f0f0f0}");
  html += F(".container{max-width:500px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}");
  html += F("h1{color:#333;text-align:center}");
  html += F(".info{background:#e7f3ff;padding:10px;border-left:4px solid #2196F3;margin:15px 0;font-size:14px}");
  html += F(".form-group{margin:15px 0}");
  html += F("label{display:block;margin-bottom:5px;color:#555;font-weight:bold}");
  html += F("input{width:100%;padding:10px;border:1px solid #ddd;border-radius:5px;box-sizing:border-box}");
  html += F("button{background:#4CAF50;color:white;padding:12px 20px;border:none;border-radius:5px;cursor:pointer;width:100%;font-size:16px;margin-top:10px}");
  html += F("button:hover{background:#45a049}");
  html += F("</style></head><body>");
  
  html += F("<div class='container'>");
  html += F("<h1>🔧 ESP8266 Config</h1>");
  html += F("<div class='info'>");
  html += F("<strong>📋 Step 1/2: Server Config</strong><br>");
  html += F("Enter Python server IP and Port");
  html += F("</div>");
  
  html += F("<form action='/server' method='POST'>");
  html += F("<div class='form-group'>");
  html += F("<label>🖥️ Server IP:</label>");
  html += F("<input type='text' name='ip' placeholder='192.168.2.60' required pattern='\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}'>");
  html += F("</div>");
  html += F("<div class='form-group'>");
  html += F("<label>🔌 Port:</label>");
  html += F("<input type='number' name='port' placeholder='8080' value='8080' required min='1' max='65535'>");
  html += F("</div>");
  html += F("<button type='submit'>Next: WiFi Config ➡️</button>");
  html += F("</form>");
  html += F("</div></body></html>");
  
  return html;
}

String ConfigPortal::generateSuccessHTML(const String& apSSID, const String& serverIP, uint16_t serverPort) {
  String html = F("<!DOCTYPE html><html><head><meta charset='UTF-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>Success</title>");
  html += F("<style>");
  html += F("body{font-family:Arial;margin:20px;background:#f0f0f0;text-align:center;padding-top:50px}");
  html += F(".container{max-width:500px;margin:0 auto;background:white;padding:40px;border-radius:10px}");
  html += F("h1{color:#28a745}");
  html += F(".success{background:#d4edda;padding:15px;border-radius:5px;margin:20px 0}");
  html += F(".step{background:#fff3cd;padding:15px;border-radius:5px;margin:20px 0}");
  html += F("</style></head><body>");
  
  html += F("<div class='container'>");
  html += F("<h1>✅ Step 1 Complete!</h1>");
  html += F("<div class='success'><strong>Server:</strong><br>");
  html += serverIP + ":" + String(serverPort);
  html += F("</div>");
  
  html += F("<div class='step'>");
  html += F("<strong>📱 Step 2: WiFi Config</strong><br><br>");
  html += F("1️⃣ ESP will reboot to WiFiManager<br>");
  html += F("2️⃣ Reconnect to: <strong>");
  html += apSSID;
  html += F("</strong><br>");
  html += F("3️⃣ Select WiFi and enter password<br>");
  html += F("</div>");
  html += F("<p>⏳ Switching to WiFiManager...</p>");
  html += F("</div>");
  html += F("<script>setTimeout(function(){window.close()},5000);</script>");
  html += F("</body></html>");
  
  return html;
}

String ConfigPortal::generateTestingHTML() {
  return F("<!DOCTYPE html><html><head><meta charset='UTF-8'></head><body><h1>Testing...</h1><p>Please wait</p></body></html>");
}

String ConfigPortal::generateErrorHTML(const char* error) {
  String html = F("<!DOCTYPE html><html><head><meta charset='UTF-8'>");
  html += F("<style>");
  html += F("body{font-family:Arial;margin:20px;text-align:center;padding-top:50px}");
  html += F(".error{background:#f8d7da;padding:15px;border-radius:5px;color:#721c24;max-width:500px;margin:0 auto}");
  html += F("</style></head><body>");
  html += F("<h1>❌ Error</h1>");
  html += F("<div class='error'>");
  html += String(error);
  html += F("</div>");
  html += F("<p><a href='/'>← Try again</a></p>");
  html += F("</body></html>");
  
  return html;
}
