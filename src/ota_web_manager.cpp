/*
 * OTA Web Manager Implementation
 */

#include "config.h"
#include "version.h"
#include "ota_web_manager.h"

OTAWebManager::OTAWebManager() 
  : display(nullptr), webServer(nullptr), httpUpdater(nullptr), 
    isActive(false), currentIP("") {}

OTAWebManager::~OTAWebManager() {
  stop();
}

void OTAWebManager::setDisplayManager(DisplayManager* dm) {
  display = dm;
}

void OTAWebManager::showActiveScreen() {
  if (display == nullptr) return;
  
  display->clear();
  display->drawText(10, 5, "FIRMWARE", ST77XX_CYAN, 2);
  display->drawText(10, 25, "UPDATE MODE", ST77XX_CYAN, 2);
  
  display->drawText(5, 55, "1. Open browser", ST77XX_WHITE, 1);
  display->drawText(5, 70, "2. Enter URL:", ST77XX_WHITE, 1);
  display->drawText(10, 85, currentIP.c_str(), ST77XX_GREEN, 1);
  display->drawText(10, 100, "/update", ST77XX_GREEN, 1);
  
  display->drawText(5, 110, "Exit: hold", ST77XX_YELLOW, 1);
  display->drawText(5, 120, "(within 3sec)", ST77XX_ORANGE, 1);
}

void OTAWebManager::showClosedScreen() {
  if (display == nullptr) return;
  
  display->clear();
  display->drawText(20, 60, "UPDATE", ST77XX_YELLOW, 2);
  display->drawText(20, 85, "MODE", ST77XX_YELLOW, 2);
  display->drawText(20, 110, "CLOSED", ST77XX_WHITE, 2);
  delay(1500);
}

String OTAWebManager::generateRootHTML() {
  String html = F("<!DOCTYPE html><html><head><meta charset='UTF-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>Firmware Update</title>");
  html += F("<style>");
  html += F("*{margin:0;padding:0;box-sizing:border-box}");
  html += F("body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;");
  html += F("background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);");
  html += F("min-height:100vh;display:flex;align-items:center;justify-content:center}");
  html += F(".container{background:white;padding:40px;border-radius:10px;");
  html += F("box-shadow:0 10px 40px rgba(0,0,0,0.2);max-width:500px;text-align:center}");
  html += F("h1{color:#333;margin-bottom:10px;font-size:28px}");
  html += F("p{color:#666;margin-bottom:30px;line-height:1.6}");
  html += F(".btn{display:inline-block;padding:15px 40px;background:#667eea;");
  html += F("color:white;text-decoration:none;border-radius:5px;font-weight:600;");
  html += F("transition:all 0.3s}");
  html += F(".btn:hover{background:#5568d3;transform:translateY(-2px)}");
  html += F(".info{background:#f8f9fa;padding:15px;border-radius:5px;");
  html += F("margin-bottom:20px;font-size:14px;color:#555}");
  html += F("</style></head><body>");
  html += F("<div class='container'>");
  html += F("<h1>Firmware Update</h1>");
  html += F("<p>");
  html += F(PROJECT_NAME);
  html += F(" v");
  html += F(PROJECT_VERSION);
  html += F("</p>");
  html += F("<div class='info'>Click the button below to access the firmware update page. ");
  html += F("You can upload a new .bin file to update your device.</div>");
  html += F("<a href='/update' class='btn'>Update Firmware</a>");
  html += F("</div></body></html>");
  return html;
}

String OTAWebManager::generateUpdateHTML() {
  String html = F("<!DOCTYPE html><html><head><meta charset='UTF-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>Upload Firmware</title>");
  html += F("<style>");
  html += F("*{margin:0;padding:0;box-sizing:border-box}");
  html += F("body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;");
  html += F("background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);");
  html += F("min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}");
  html += F(".container{background:white;padding:40px;border-radius:10px;");
  html += F("box-shadow:0 10px 40px rgba(0,0,0,0.2);max-width:600px;width:100%}");
  html += F("h1{color:#333;margin-bottom:10px;font-size:28px;text-align:center}");
  html += F("p{color:#666;margin-bottom:30px;line-height:1.6;text-align:center}");
  html += F(".upload-area{border:2px dashed #667eea;border-radius:8px;padding:40px;");
  html += F("text-align:center;cursor:pointer;transition:all 0.3s;margin-bottom:20px}");
  html += F(".upload-area:hover{border-color:#5568d3;background:#f8f9ff}");
  html += F(".upload-area.dragover{border-color:#5568d3;background:#e8eaff}");
  html += F("input[type=file]{display:none}");
  html += F(".file-icon{font-size:48px;color:#667eea;margin-bottom:15px}");
  html += F(".file-name{color:#333;font-weight:600;margin:10px 0}");
  html += F(".file-size{color:#999;font-size:14px}");
  html += F(".btn{display:block;width:100%;padding:15px;background:#667eea;color:white;");
  html += F("border:none;border-radius:5px;font-weight:600;font-size:16px;cursor:pointer;");
  html += F("transition:all 0.3s;margin-top:20px}");
  html += F(".btn:hover:not(:disabled){background:#5568d3;transform:translateY(-2px)}");
  html += F(".btn:disabled{background:#ccc;cursor:not-allowed}");
  html += F(".progress{display:none;margin-top:20px}");
  html += F(".progress-bar{height:30px;background:#f0f0f0;border-radius:15px;overflow:hidden}");
  html += F(".progress-fill{height:100%;background:linear-gradient(90deg,#667eea,#764ba2);");
  html += F("transition:width 0.3s;display:flex;align-items:center;justify-content:center;");
  html += F("color:white;font-weight:600}");
  html += F(".status{text-align:center;margin-top:15px;color:#666;font-size:14px}");
  html += F(".success{color:#28a745;font-weight:600}");
  html += F(".error{color:#dc3545;font-weight:600}");
  html += F("</style></head><body>");
  html += F("<div class='container'>");
  html += F("<h1>Upload Firmware</h1>");
  html += F("<p>Select a .bin file to update your device</p>");
  html += F("<form id='uploadForm' method='POST' enctype='multipart/form-data'>");
  html += F("<div class='upload-area' id='uploadArea'>");
  html += F("<div class='file-icon'>📁</div>");
  html += F("<div id='fileName'>Click or drag file here</div>");
  html += F("<div id='fileSize' class='file-size'></div>");
  html += F("<input type='file' id='fileInput' name='firmware' accept='.bin'>");
  html += F("</div>");
  html += F("<button type='submit' class='btn' id='uploadBtn' disabled>Upload Firmware</button>");
  html += F("</form>");
  html += F("<div class='progress' id='progress'>");
  html += F("<div class='progress-bar'><div class='progress-fill' id='progressFill'>0%</div></div>");
  html += F("<div class='status' id='status'></div>");
  html += F("</div>");
  html += F("</div>");
  html += F("<script>");
  html += F("const area=document.getElementById('uploadArea');");
  html += F("const input=document.getElementById('fileInput');");
  html += F("const btn=document.getElementById('uploadBtn');");
  html += F("const form=document.getElementById('uploadForm');");
  html += F("const fileName=document.getElementById('fileName');");
  html += F("const fileSize=document.getElementById('fileSize');");
  html += F("const progress=document.getElementById('progress');");
  html += F("const progressFill=document.getElementById('progressFill');");
  html += F("const status=document.getElementById('status');");
  html += F("area.onclick=()=>input.click();");
  html += F("input.onchange=e=>{const f=e.target.files[0];");
  html += F("if(f){fileName.textContent=f.name;fileSize.textContent=(f.size/1024).toFixed(1)+'KB';");
  html += F("btn.disabled=false}};");
  html += F("['dragover','dragenter'].forEach(e=>area.addEventListener(e,ev=>{");
  html += F("ev.preventDefault();area.classList.add('dragover')}));");
  html += F("['dragleave','drop'].forEach(e=>area.addEventListener(e,()=>area.classList.remove('dragover')));");
  html += F("area.addEventListener('drop',e=>{e.preventDefault();");
  html += F("const f=e.dataTransfer.files[0];if(f&&f.name.endsWith('.bin')){");
  html += F("input.files=e.dataTransfer.files;fileName.textContent=f.name;");
  html += F("fileSize.textContent=(f.size/1024).toFixed(1)+'KB';btn.disabled=false}});");
  html += F("form.onsubmit=async e=>{e.preventDefault();");
  html += F("const f=input.files[0];if(!f)return;");
  html += F("btn.disabled=true;progress.style.display='block';");
  html += F("const fd=new FormData();fd.append('firmware',f);");
  html += F("const xhr=new XMLHttpRequest();");
  html += F("xhr.upload.onprogress=e=>{if(e.lengthComputable){");
  html += F("const p=Math.round(e.loaded/e.total*100);");
  html += F("progressFill.style.width=p+'%';progressFill.textContent=p+'%';");
  html += F("status.textContent='Uploading...'}};");
  html += F("xhr.onload=()=>{if(xhr.status===200){");
  html += F("progressFill.style.width='100%';progressFill.textContent='100%';");
  html += F("status.textContent='Upload complete! Rebooting...';status.className='status success';");
  html += F("setTimeout(()=>location.href='/',3000)}else{");
  html += F("status.textContent='Upload failed!';status.className='status error';btn.disabled=false}};");
  html += F("xhr.onerror=()=>{status.textContent='Upload error!';status.className='status error';btn.disabled=false};");
  html += F("xhr.open('POST','/upload');xhr.send(fd)};");
  html += F("</script></body></html>");
  return html;
}

void OTAWebManager::handleUpdatePage() {
  webServer->send(200, "text/html", generateUpdateHTML());
}

void OTAWebManager::handleUpload() {
  HTTPUpload& upload = webServer->upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("[OTA] Upload started: %s\n", upload.filename.c_str());
    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin(maxSketchSpace)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("[OTA] Upload success: %u bytes\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

void OTAWebManager::handleUploadCallback() {
  webServer->send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
  if (!Update.hasError()) {
    delay(1000);
    ESP.restart();
  }
}

void OTAWebManager::start(const String& ipAddress) {
  if (isActive) return;
  
  currentIP = ipAddress;
  
  DEBUG_PRINTLN(F("\n[OTA] Entering OTA Update Mode..."));
  
  // Show active screen
  showActiveScreen();
  
  // Initialize web server
  webServer = new ESP8266WebServer(80);
  
  // Root page
  webServer->on("/", [this]() {
    webServer->send(200, "text/html", generateRootHTML());
  });
  
  // Custom update page
  webServer->on("/update", HTTP_GET, [this]() {
    handleUpdatePage();
  });
  
  // Handle firmware upload
  webServer->on("/upload", HTTP_POST, [this]() {
    handleUploadCallback();
  }, [this]() {
    handleUpload();
  });
  
  webServer->begin();
  isActive = true;
  
  Serial.println(F("[OTA] Mode activated - Access: http://"));
  Serial.print(currentIP);
  Serial.println(F("/update"));
}

void OTAWebManager::stop() {
  if (!isActive) return;
  
  DEBUG_PRINTLN(F("\n[OTA] Exiting OTA Mode..."));
  
  // Show closed screen
  showClosedScreen();
  
  // Clean up
  if (webServer != nullptr) {
    webServer->stop();
    delete webServer;
    webServer = nullptr;
  }
  
  if (httpUpdater != nullptr) {
    delete httpUpdater;
    httpUpdater = nullptr;
  }
  
  isActive = false;
  
  DEBUG_PRINTLN(F("[OTA] Mode closed"));
  DEBUG_PRINTLN(F("[OTA] Returning to normal operation"));
}

void OTAWebManager::handle() {
  if (isActive && webServer != nullptr) {
    webServer->handleClient();
    yield();
  }
}

bool OTAWebManager::active() const {
  return isActive;
}
