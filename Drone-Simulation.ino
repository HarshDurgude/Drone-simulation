#include <WiFi.h>
#include <WebServer.h>

// LED pins
#define FL 25
#define FR 26
#define BL 27
#define BR 14

// Wi-Fi AP
const char* ssid = "ESP32-Drone";
const char* password = "12345678";

WebServer server(80);

// Control state
int throttleLevel = 0;   // 0–5
int pitch = 0;           // -1 back, 0 neutral, +1 forward
int roll  = 0;           // -1 left, 0 neutral, +1 right

// Brightness mapping
int basePWM() {
  return throttleLevel * 45; // 0–225
}

void setup() {
  ledcAttach(FL, 5000, 8);
  ledcAttach(FR, 5000, 8);
  ledcAttach(BL, 5000, 8);
  ledcAttach(BR, 5000, 8);

  WiFi.softAP(ssid, password);

  server.on("/", handleUI);

  server.on("/throttle_up", []() {
    if (throttleLevel < 5) throttleLevel++;
    updateLEDs();
    handleUI();
  });

  server.on("/throttle_down", []() {
    if (throttleLevel > 0) throttleLevel--;
    updateLEDs();
    handleUI();
  });

  server.on("/start", []() {
    throttleLevel = 1;
    updateLEDs();
    handleUI();
  });

  server.on("/stop", []() {
    throttleLevel = 0;
    pitch = roll = 0;
    updateLEDs();
    handleUI();
  });

  // Direction press
  server.on("/fwd_on", [](){ pitch = 1; updateLEDs(); server.send(200); });
  server.on("/back_on", [](){ pitch = -1; updateLEDs(); server.send(200); });
  server.on("/left_on", [](){ roll = -1; updateLEDs(); server.send(200); });
  server.on("/right_on", [](){ roll = 1; updateLEDs(); server.send(200); });

  // Direction release
  server.on("/dir_off", [](){
    pitch = roll = 0;
    updateLEDs();
    server.send(200);
  });

  server.begin();
}

void loop() {
  server.handleClient();
}

void updateLEDs() {
  int base = basePWM();
  int boost = 80;   // jerk strength

  int fl = base;
  int fr = base;
  int bl = base;
  int br = base;

  // Pitch
  if (pitch == 1) {       // forward
    bl += boost; br += boost;
    fl -= boost; fr -= boost;
  } else if (pitch == -1) { // backward
    fl += boost; fr += boost;
    bl -= boost; br -= boost;
  }

  // Roll
  if (roll == 1) {        // right
    fl += boost; bl += boost;
    fr -= boost; br -= boost;
  } else if (roll == -1) { // left
    fr += boost; br += boost;
    fl -= boost; bl -= boost;
  }

  fl = constrain(fl, 0, 255);
  fr = constrain(fr, 0, 255);
  bl = constrain(bl, 0, 255);
  br = constrain(br, 0, 255);

  ledcWrite(FL, fl);
  ledcWrite(FR, fr);
  ledcWrite(BL, bl);
  ledcWrite(BR, br);
}

// ================= UI =================
void handleUI() {
  String html = "";
  html += "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Drone Simulator</title>";

  html += "<style>";
  html += "*{-webkit-user-select:none;-moz-user-select:none;-ms-user-select:none;";
  html += "user-select:none;-webkit-touch-callout:none;}";
  html += "body{background:#0f0f0f;color:white;font-family:Arial;text-align:center;}";
  html += "button{width:90px;height:70px;font-size:18px;margin:8px;";
  html += "border:none;border-radius:14px;background:#333;color:white;}";
  html += ".start{background:#2ecc71;}";
  html += ".stop{background:#e74c3c;}";
  html += ".arrow{background:#444;}";
  html += ".level{font-size:28px;margin:10px;}";
  html += "</style>";


  html += "<script>";
  html += "function vibrate(){ if(navigator.vibrate) navigator.vibrate(30); }";
  html += "</script>";

  html += "</head><body>";

  html += "<h2>ESP32 Drone Control</h2>";

  html += "<button class='start' onclick=\"vibrate();location.href='/start'\">START</button>";
  html += "<button class='stop' onclick=\"vibrate();location.href='/stop'\">STOP</button>";

  html += "<div>";
  html += "<button class='arrow' onclick=\"vibrate();location.href='/throttle_up'\">+</button>";
  html += "<button class='arrow' onclick=\"vibrate();location.href='/throttle_down'\">-</button>";
  html += "</div>";

  html += "<div class='level'>Throttle Level: ";
  html += throttleLevel;
  html += "</div>";

  html += "<div>";
  html += "<button class='arrow' ontouchstart=\"vibrate();fetch('/fwd_on')\" ontouchend=\"fetch('/dir_off')\">UP</button>";
  html += "</div>";

  html += "<div>";
  html += "<button class='arrow' ontouchstart=\"vibrate();fetch('/left_on')\" ontouchend=\"fetch('/dir_off')\">LEFT</button>";
  html += "<button class='arrow' ontouchstart=\"vibrate();fetch('/right_on')\" ontouchend=\"fetch('/dir_off')\">RIGHT</button>";
  html += "</div>";

  html += "<div>";
  html += "<button class='arrow' ontouchstart=\"vibrate();fetch('/back_on')\" ontouchend=\"fetch('/dir_off')\">DOWN</button>";
  html += "</div>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

