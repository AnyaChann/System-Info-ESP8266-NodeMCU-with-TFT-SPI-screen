/*
 * Config Portal HTML Implementation
 */

#include "config_portal.h"

String ConfigPortal::generateServerConfigHTML() {
  String html = F("<!DOCTYPE html><html><head><meta charset='UTF-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>ESP8266 Configuration</title>");
  html += F("<style>");
  html += F("*{box-sizing:border-box;margin:0;padding:0}");
  html += F("body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;padding:20px}");
  html += F(".container{max-width:600px;margin:40px auto;background:white;border-radius:12px;box-shadow:0 8px 32px rgba(0,0,0,0.1);overflow:hidden}");
  html += F(".header{background:#2c3e50;color:white;padding:24px;text-align:center}");
  html += F(".header h1{font-size:24px;font-weight:600;margin-bottom:8px}");
  html += F(".header p{font-size:14px;opacity:0.9}");
  html += F(".content{padding:32px}");
  html += F(".step-indicator{background:#f8f9fa;padding:12px 16px;border-radius:8px;margin-bottom:24px;border-left:4px solid #667eea}");
  html += F(".step-indicator strong{display:block;color:#2c3e50;margin-bottom:4px}");
  html += F(".step-indicator span{font-size:13px;color:#6c757d}");
  html += F(".guide{background:#f8f9fa;padding:20px;border-radius:8px;margin-bottom:24px}");
  html += F(".guide-title{font-weight:600;color:#2c3e50;margin-bottom:12px;font-size:15px}");
  html += F(".guide-steps{font-size:13px;line-height:1.8;color:#495057}");
  html += F(".guide-steps ol{margin-left:20px}");
  html += F(".guide-steps li{margin-bottom:8px}");
  html += F(".guide-steps code{background:#e9ecef;padding:2px 8px;border-radius:4px;font-family:monospace;font-size:12px}");
  html += F(".form-group{margin-bottom:20px}");
  html += F("label{display:block;margin-bottom:8px;color:#2c3e50;font-weight:500;font-size:14px}");
  html += F("input{width:100%;padding:12px 16px;border:2px solid #e9ecef;border-radius:8px;font-size:14px;transition:border 0.3s}");
  html += F("input:focus{outline:none;border-color:#667eea}");
  html += F(".example{font-size:12px;color:#6c757d;margin-top:4px}");
  html += F("button{background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:white;padding:14px;border:none;border-radius:8px;cursor:pointer;width:100%;font-size:15px;font-weight:600;margin-top:8px;transition:transform 0.2s}");
  html += F("button:hover{transform:translateY(-2px)}");
  html += F("</style></head><body>");
  
  html += F("<div class='container'>");
  html += F("<div class='header'>");
  html += F("<h1>ESP8266 System Monitor</h1>");
  html += F("<p>Server Configuration Setup</p>");
  html += F("</div>");
  
  html += F("<div class='content'>");
  html += F("<div class='step-indicator'>");
  html += F("<strong>Step 1 of 2</strong>");
  html += F("<span>Configure Python server connection</span>");
  html += F("</div>");
  
  html += F("<div class='guide'>");
  html += F("<div class='guide-title'>How to configure Server Connection:</div>");
  html += F("<div class='guide-steps'>");
  html += F("<ol>");
  html += F("<li><strong>Local Network:</strong> Use IP address + Port<br><code>192.168.1.100</code> with port <code>8080</code></li>");
  html += F("<li><strong>Public Domain:</strong> Use URL only<br><code>example.com</code> (leave port empty for :80)</li>");
  html += F("<li><strong>Custom Port:</strong> Use domain + Port<br><code>example.com</code> with port <code>8080</code></li>");
  html += F("<li>Port is optional - defaults to <code>80</code> if empty</li>");
  html += F("</ol>");
  html += F("</div>");
  html += F("</div>");
  
  html += F("<form action='/server' method='POST'>");
  html += F("<div class='form-group'>");
  html += F("<label>Server Address (IP or Domain)</label>");
  html += F("<input type='text' name='ip' placeholder='192.168.1.100 or example.com' required>");
  html += F("<div class='example'>Examples: 192.168.1.100, abcd.xyz, server.local</div>");
  html += F("</div>");
  html += F("<div class='form-group'>");
  html += F("<label>Server Port (optional)</label>");
  html += F("<input type='number' name='port' placeholder='80 (default)' value='' min='1' max='65535'>");
  html += F("<div class='example'>Leave empty for port 80, or enter custom port (e.g. 8080)</div>");
  html += F("</div>");
  html += F("<button type='submit'>Continue to WiFi Setup</button>");
  html += F("</form>");
  html += F("</div>");
  html += F("</div></body></html>");
  
  return html;
}

String ConfigPortal::generateSuccessHTML(const String& apSSID, const String& serverIP, uint16_t serverPort) {
  String html = F("<!DOCTYPE html><html><head><meta charset='UTF-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>Configuration Saved</title>");
  html += F("<style>");
  html += F("*{box-sizing:border-box;margin:0;padding:0}");
  html += F("body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:linear-gradient(135deg,#11998e 0%,#38ef7d 100%);min-height:100vh;padding:20px;display:flex;align-items:center;justify-content:center}");
  html += F(".container{max-width:500px;background:white;border-radius:12px;box-shadow:0 8px 32px rgba(0,0,0,0.1);overflow:hidden}");
  html += F(".header{background:#28a745;color:white;padding:32px;text-align:center}");
  html += F(".header h1{font-size:24px;font-weight:600;margin-bottom:8px}");
  html += F(".check{width:60px;height:60px;border:3px solid white;border-radius:50%;margin:0 auto 16px;position:relative}");
  html += F(".check:after{content:'';position:absolute;width:12px;height:24px;border:solid white;border-width:0 3px 3px 0;top:12px;left:20px;transform:rotate(45deg)}");
  html += F(".content{padding:32px;text-align:center}");
  html += F(".config-box{background:#f8f9fa;padding:16px;border-radius:8px;margin-bottom:24px}");
  html += F(".config-label{font-size:12px;color:#6c757d;text-transform:uppercase;letter-spacing:0.5px;margin-bottom:4px}");
  html += F(".config-value{font-size:18px;color:#2c3e50;font-weight:600}");
  html += F(".next-steps{background:#fff3cd;padding:20px;border-radius:8px;text-align:left}");
  html += F(".next-steps-title{font-weight:600;color:#856404;margin-bottom:12px}");
  html += F(".next-steps ol{margin-left:20px;color:#856404}");
  html += F(".next-steps li{margin-bottom:8px;line-height:1.6}");
  html += F(".next-steps strong{color:#2c3e50}");
  html += F(".status{color:#6c757d;font-size:14px;margin-top:20px}");
  html += F("</style></head><body>");
  
  html += F("<div class='container'>");
  html += F("<div class='header'>");
  html += F("<div class='check'></div>");
  html += F("<h1>Configuration Saved</h1>");
  html += F("<p>Step 1 completed successfully</p>");
  html += F("</div>");
  
  html += F("<div class='content'>");
  html += F("<div class='config-box'>");
  html += F("<div class='config-label'>Server Connection</div>");
  html += F("<div class='config-value'>");
  // Show port only if not default (80)
  if (serverPort == 80) {
    html += serverIP;
  } else {
    html += serverIP + ":" + String(serverPort);
  }
  html += F("</div>");
  html += F("</div>");
  
  html += F("<div class='next-steps'>");
  html += F("<div class='next-steps-title'>Next: WiFi Configuration</div>");
  html += F("<ol>");
  html += F("<li>Device will reboot to WiFi setup mode</li>");
  html += F("<li>Reconnect to access point: <strong>");
  html += apSSID;
  html += F("</strong></li>");
  html += F("<li>Select your WiFi network</li>");
  html += F("<li>Enter WiFi password</li>");
  html += F("<li>Configuration complete</li>");
  html += F("</ol>");
  html += F("</div>");
  
  html += F("<div class='status'>Switching to WiFi setup mode...</div>");
  html += F("</div>");
  html += F("</div>");
  html += F("<script>setTimeout(function(){window.close()},6000);</script>");
  html += F("</body></html>");
  
  return html;
}

String ConfigPortal::generateTestingHTML() {
  String html = F("<!DOCTYPE html><html><head><meta charset='UTF-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>Validating</title>");
  html += F("<style>");
  html += F("*{box-sizing:border-box;margin:0;padding:0}");
  html += F("body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;display:flex;align-items:center;justify-content:center}");
  html += F(".container{text-align:center;color:white}");
  html += F(".spinner{width:60px;height:60px;border:4px solid rgba(255,255,255,0.3);border-top-color:white;border-radius:50%;animation:spin 1s linear infinite;margin:0 auto 24px}");
  html += F("@keyframes spin{to{transform:rotate(360deg)}}");
  html += F("h1{font-size:24px;font-weight:600;margin-bottom:8px}");
  html += F("p{font-size:14px;opacity:0.9}");
  html += F("</style></head><body>");
  html += F("<div class='container'>");
  html += F("<div class='spinner'></div>");
  html += F("<h1>Validating Configuration</h1>");
  html += F("<p>Please wait while we test the connection...</p>");
  html += F("</div></body></html>");
  return html;
}

String ConfigPortal::generateErrorHTML(const char* error) {
  String html = F("<!DOCTYPE html><html><head><meta charset='UTF-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>Configuration Error</title>");
  html += F("<style>");
  html += F("*{box-sizing:border-box;margin:0;padding:0}");
  html += F("body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:linear-gradient(135deg,#f093fb 0%,#f5576c 100%);min-height:100vh;padding:20px;display:flex;align-items:center;justify-content:center}");
  html += F(".container{max-width:500px;background:white;border-radius:12px;box-shadow:0 8px 32px rgba(0,0,0,0.1);overflow:hidden}");
  html += F(".header{background:#dc3545;color:white;padding:32px;text-align:center}");
  html += F(".header h1{font-size:24px;font-weight:600;margin-bottom:8px}");
  html += F(".cross{width:60px;height:60px;border:3px solid white;border-radius:50%;margin:0 auto 16px;position:relative}");
  html += F(".cross:before,.cross:after{content:'';position:absolute;width:3px;height:30px;background:white;top:15px;left:28px}");
  html += F(".cross:before{transform:rotate(45deg)}");
  html += F(".cross:after{transform:rotate(-45deg)}");
  html += F(".content{padding:32px;text-align:center}");
  html += F(".error-box{background:#f8d7da;padding:20px;border-radius:8px;margin-bottom:24px;border-left:4px solid #dc3545}");
  html += F(".error-message{color:#721c24;font-size:14px;line-height:1.6}");
  html += F(".retry-btn{display:inline-block;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:white;padding:12px 32px;border-radius:8px;text-decoration:none;font-weight:600;transition:transform 0.2s}");
  html += F(".retry-btn:hover{transform:translateY(-2px)}");
  html += F("</style></head><body>");
  html += F("<div class='container'>");
  html += F("<div class='header'>");
  html += F("<div class='cross'></div>");
  html += F("<h1>Configuration Error</h1>");
  html += F("<p>Unable to complete setup</p>");
  html += F("</div>");
  html += F("<div class='content'>");
  html += F("<div class='error-box'>");
  html += F("<div class='error-message'>");
  html += String(error);
  html += F("</div>");
  html += F("</div>");
  html += F("<a href='/' class='retry-btn'>Try Again</a>");
  html += F("</div>");
  html += F("</div></body></html>");
  
  return html;
}
