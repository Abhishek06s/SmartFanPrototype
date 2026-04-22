#include <WiFi.h>
#include <WebServer.h>

#define SERVO_PIN 26
#define SIGNAL_PIN 14

// 🔴 DC MOTOR PINS
#define IN1 27
#define IN2 25
#define ENA 33

#define TEMP_PIN 34   // from ESP32 #1 (DAC)

// --- WiFi Settings ---
const char* ssid = "SmartFan_Pro";
const char* password = "password123";

WebServer server(80);

unsigned long moveStartTime = 0;
unsigned long movedTime = 0;

const unsigned long maxScanTime = 780;
const unsigned long holdTime = 20000;

float currentAngle = 90.0;     // Persistent angle tracker
int sweepDirection = 0;        // 0 = increasing (to 135), 1 = decreasing (to 45)
unsigned long lastUpdateTick = 0;

int direction = 0;
bool stopped = false;

unsigned long lastDetectionTime = 0;

// Global variables to store current state for web server
float currentTemp = 0.0;
int currentSpeed = 0;
int currentMotion = 0;
float filteredTemp = 27.0; // Initial seed for EMA filter

// --- HTML Dashboard (Stored in Flash memory) ---
const char index_html[] PROGMEM = R"rawtext(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Fan Dashboard</title>
    <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;600&display=swap" rel="stylesheet">
    <style>
        :root { --primary: #00f2fe; --secondary: #4facfe; --bg: #0a0b1e; --card-bg: rgba(255, 255, 255, 0.05); --danger: #ff4757; --success: #2ed573; }
        * { margin:0; padding:0; box-sizing:border-box; font-family:'Outfit',sans-serif; }
        body { background:var(--bg); color:white; min-height:100vh; display:flex; flex-direction:column; align-items:center; overflow-x:hidden; background:radial-gradient(circle at top right, #1e2a4a, #0a0b1e); }
        .bg-glow { position:fixed; top:0; left:0; width:100%; height:100%; z-index:-1; overflow:hidden; }
        .glow-orb { position:absolute; border-radius:50%; filter:blur(80px); opacity:0.15; animation:move 20s infinite alternate; }
        @keyframes move { from { transform: translate(0,0); } to { transform: translate(100px, 100px); } }
        nav { width:100%; padding:2rem; text-align:center; background:rgba(0,0,0,0.2); backdrop-filter:blur(10px); border-bottom:1px solid rgba(255,255,255,0.1); }
        h1 { font-weight:600; letter-spacing:2px; background:linear-gradient(to right, var(--primary), var(--secondary)); -webkit-background-clip:text; -webkit-text-fill-color:transparent; font-size:2rem; }
        .container { flex:1; width:100%; max-width:1200px; padding:2rem; display:grid; grid-template-columns:repeat(auto-fit, minmax(300px, 1fr)); gap:2rem; align-items:start; }
        .card { background:var(--card-bg); backdrop-filter:blur(20px); border-radius:34px; padding:2rem; border:1px solid rgba(255,255,255,0.1); transition:all 0.3s ease; display:flex; flex-direction:column; align-items:center; justify-content:center; height:400px; }
        .card:hover { transform:translateY(-5px); box-shadow:0 10px 30px rgba(0,0,0,0.3); border-color:rgba(0,242,254,0.3); }
        .card-title { font-size:0.9rem; text-transform:uppercase; letter-spacing:1.5px; color:rgba(255,255,255,0.5); margin-bottom:1.5rem; }
        
        /* High-End Speedometer */
        .speedometer-wrapper { position:relative; width:220px; height:130px; margin-bottom:1rem; }
        .speed-gauge { width:100%; height:100%; filter:drop-shadow(0 0 10px rgba(0, 242, 254, 0.2)); }
        .glowing-needle { position:absolute; bottom:8px; left:calc(50% - 2px); width:4px; height:85px; background:#fff; box-shadow:0 0 10px #00f2fe, 0 0 20px #00f2fe; border-radius:4px; transform-origin:bottom center; transform:rotate(-90deg); transition:transform 0.8s cubic-bezier(0.17, 0.67, 0.83, 0.67); z-index:2; }
        .needle-base { position:absolute; bottom:-4px; left:calc(50% - 12px); width:24px; height:24px; background:#0a0b1e; border:2px solid var(--primary); border-radius:50%; box-shadow: 0 0 15px rgba(0, 242, 254, 0.5); z-index:3; }
        .speed-val { font-size:3.5rem; font-weight:700; background:linear-gradient(to right, #fff, #ddd); -webkit-background-clip:text; -webkit-text-fill-color:transparent; line-height:1; }
        .speed-unit { font-size:0.8rem; color:rgba(255,255,255,0.4); text-transform:uppercase; letter-spacing:2px; }

        .temp-display { font-size:4rem; font-weight:600; margin:0.5rem 0; text-shadow:0 0 20px rgba(79,172,254,0.3); }
        .status-indicator { width:80px; height:80px; border-radius:50%; background:rgba(255,255,255,0.05); margin-bottom:1.5rem; display:flex; align-items:center; justify-content:center; position:relative; border:2px solid rgba(255,255,255,0.1); }
        .status-dot { width:36px; height:36px; border-radius:50%; background:var(--danger); transition:0.3s; box-shadow:0 0 15px var(--danger); }
        .status-active .status-dot { background:var(--success); box-shadow:0 0 30px var(--success); animation:pulse 1.5s infinite; }
        @keyframes pulse { 0% { transform:scale(1); opacity:1; } 50% { transform:scale(1.15); opacity:0.8; } 100% { transform:scale(1); opacity:1; } }
        .motion-label { font-size:1.2rem; font-weight:600; }
        .footer { padding:2rem; font-size:0.8rem; color:rgba(255,255,255,0.3); }
    </style>
</head>
<body>
    <div class="bg-glow">
        <div class="glow-orb" style="width:400px;height:400px;background:var(--primary);top:10%;right:10%;"></div>
        <div class="glow-orb" style="width:300px;height:300px;background:rgba(255,8,68,0.5);bottom:10%;left:10%;"></div>
    </div>
    <nav><h1>SMART FAN SYSTEM</h1></nav>
    <div class="container">
        <div class="card">
            <div class="card-title">Motor Intensity</div>
            <div class="speedometer-wrapper">
                <svg viewBox="0 0 250 150" class="speed-gauge">
                    <defs>
                        <linearGradient id="speedGrad" x1="0%" y1="0%" x2="100%" y2="0%">
                            <stop offset="0%" stop-color="#00f2fe"/><stop offset="100%" stop-color="#ff0844"/>
                        </linearGradient>
                    </defs>
                    <path d="M 25 140 A 100 100 0 0 1 225 140" fill="none" stroke="rgba(255,255,255,0.05)" stroke-width="18" stroke-linecap="round"/>
                    <path id="speedProgress" d="M 25 140 A 100 100 0 0 1 225 140" fill="none" stroke="url(#speedGrad)" stroke-width="18" stroke-linecap="round" stroke-dasharray="314.16" stroke-dashoffset="314.16" style="transition:stroke-dashoffset 0.8s cubic-bezier(0.17, 0.67, 0.83, 0.67);"/>
                </svg>
                <div class="glowing-needle" id="needle"></div>
                <div class="needle-base"></div>
            </div>
            <div style="text-align:center; margin-top:-10px;">
                <span class="speed-val" id="speedText">0</span><br>
                <span class="speed-unit">PWM OUTPUT</span>
            </div>
        </div>
        <div class="card">
            <div class="card-title">Environment</div>
            <div class="temp-display"><span id="tempText">0.0</span>°</div>
            <p style="color:rgba(255,255,255,0.6)">Current Temperature</p>
        </div>
        <div class="card">
            <div class="card-title">Security State</div>
            <div class="status-indicator" id="motionIndicator"><div class="status-dot"></div></div>
            <div class="motion-label" id="motionText">STDBY</div>
        </div>
    </div>
    <div class="footer">Connected via SmartFan Access Point • Live Telemetry</div>
    <script>
        function updateGauge(val) {
            const rotation = (val / 255) * 180 - 90;
            document.getElementById('needle').style.transform = `rotate(${rotation}deg)`;
            const dashoffset = 314.16 - (val / 255) * 314.16;
            document.getElementById('speedProgress').style.strokeDashoffset = dashoffset;
            document.getElementById('speedText').innerText = val;
        }
        async function updateData() {
            try {
                const response = await fetch('/data');
                const data = await response.json();
                document.getElementById('tempText').innerText = data.temp.toFixed(1);
                updateGauge(data.speed);
                const indicator = document.getElementById('motionIndicator');
                const text = document.getElementById('motionText');
                if (data.motion) {
                    indicator.classList.add('status-active');
                    text.innerText = "MOTION DETECTED";
                    text.style.color = "var(--success)";
                } else {
                    indicator.classList.remove('status-active');
                    text.innerText = "STDBY";
                    text.style.color = "white";
                }
            } catch (e) { console.log("Reconnecting..."); }
        }
        setInterval(updateData, 1000);
    </script>
</body>
</html>
)rawtext";

// --- Web Handlers ---
void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleData() {
  String json = "{";
  json += "\"temp\":" + String(currentTemp) + ",";
  json += "\"speed\":" + String(currentSpeed) + ",";
  json += "\"motion\":" + String(currentMotion);
  json += "}";
  server.send(200, "application/json", json);
}

// --- Servo ---
void setServoAngle(int angle) {
  angle = constrain(angle, 0, 180);
  long pulse_us = map(angle, 0, 180, 500, 2500);
  uint32_t duty = (pulse_us * 65535) / 20000;
  ledcWrite(SERVO_PIN, duty);
}

// --- Motor ---
void runMotor(int speedVal) {
  digitalWrite(IN1, HIGH);''
  digitalWrite(IN2, LOW);
  analogWrite(ENA, speedVal);
}

void stopMotor() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 0);
}

void setup() {
  Serial.begin(115200);

  // 🌐 WiFi Setup (Access Point Mode)
  WiFi.softAP(ssid, password);
  Serial.print("Access Point SSID: ");
  Serial.println(ssid);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // 🛠 Server Endpoints
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();

  pinMode(SIGNAL_PIN, INPUT);
  pinMode(TEMP_PIN, INPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);

  ledcAttach(SERVO_PIN, 50, 16);

  setServoAngle(91);
  delay(500);
  
  // Initialize tracking variables
  currentAngle = 90.0;
  sweepDirection = 0;
  lastUpdateTick = millis();

  stopMotor();

  Serial.println("System Ready - Web Dashboard Hosted");
}

void loop() {
  server.handleClient(); // Handle web requests

  int motion = digitalRead(SIGNAL_PIN);
  int adcValue = analogRead(TEMP_PIN);

  // 🌡️ TEMPERATURE MAPPING & SMOOTHING
  // Calibrated for ESP32: DAC (3.1V) to ADC (11dB) mismatch
  // Maps 0 - 3640 ADC range to 20°C - 40°C
  float adcMaxRange = 3640.0; 
  float rawTemperature = 20.0 + (adcValue / adcMaxRange) * 20.0;
  
  // EMA Filter (Exponential Moving Average) to stabilize fan speed
  filteredTemp = (filteredTemp * 0.9) + (rawTemperature * 0.1); 

  int motorSpeed;
  if (filteredTemp < 25) motorSpeed = 80;
  else if (filteredTemp < 30) motorSpeed = 180;
  else if (filteredTemp < 32.5) motorSpeed = 200;
  else motorSpeed = 255;

  // Update global variables for server
  currentTemp = filteredTemp;
  currentMotion = motion;

  // 🔍 DEBUG TELEMETRY (Every 1 second)
  static unsigned long lastDebugPrint = 0;
  if (millis() - lastDebugPrint > 1000) {
    Serial.printf("ADC: %d | RAW T: %.1fC | SMOOTH T: %.1fC | MOT: %s | HOLD: %ds\n", 
                  adcValue, rawTemperature, filteredTemp, 
                  (motion == HIGH ? "DETECT" : "STDBY"),
                  (stopped ? (int)((holdTime - (millis() - lastDetectionTime))/1000) : 0));
    lastDebugPrint = millis();
  }

  // 🎯 MOTION & FAN CONTROL LOGIC
  if (motion == HIGH) {
    lastDetectionTime = millis(); // Reset 20s hold timer on every detection
    
    if (!stopped) {
      // First detection: Lock the current angle immediately
      setServoAngle((int)currentAngle);
      delay(150); 
      ledcWrite(SERVO_PIN, 0); // Lock servo power
      stopped = true;
    }
  }

  if (stopped) {
    // Fan ON during hold
    runMotor(motorSpeed);
    currentSpeed = motorSpeed;

    if (millis() - lastDetectionTime >= holdTime) {
      stopped = false;
      stopMotor();
      currentSpeed = 0;
      lastUpdateTick = millis(); // Synchronize clock for resume
    }
  } else {
    // 🔄 SMOOTH SCANNING (Jump-free)
    stopMotor();
    currentSpeed = 0;
    
    unsigned long dt = millis() - lastUpdateTick;
    lastUpdateTick = millis();

    // Calculate movement based on time passed
    // 90 degrees per 780ms = approx 0.115 deg/ms
    float step = (90.0 / (float)maxScanTime) * (float)dt;

    if (sweepDirection == 0) currentAngle += step;
    else currentAngle -= step;

    // Bounds check and turn-around
    if (currentAngle >= 135.0) {
      currentAngle = 135.0;
      sweepDirection = 1;
    } 
    else if (currentAngle <= 45.0) {
      currentAngle = 45.0;
      sweepDirection = 0;
    }

    setServoAngle((int)currentAngle);
  }
}
