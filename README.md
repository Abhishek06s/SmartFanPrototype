# 🌬️ Smart Adaptive Fan System

An IoT-enabled smart fan that automatically adjusts its **speed** and **direction** based on **temperature** and **human presence** using dual ESP32 microcontrollers.

---

## 🚀 Features

- 👤 Human presence detection using PIR sensor  
- 🌡️ Temperature-based automatic speed control  
- 🔄 Servo-based dynamic fan orientation  
- ⚡ Energy-efficient operation  
- 🌐 Built-in web dashboard (ESP32 Access Point)  
- ⏱️ Smart hold logic to prevent frequent switching  
- 📡 Dual ESP32 architecture for reliable sensing and control  

---

## 🧠 System Architecture

### ESP32 #1 – Sensor Node
- Reads:
  - DHT22 temperature sensor  
  - PIR motion sensor  
- Outputs:
  - Analog temperature signal (DAC → ESP32 #2 ADC)  
  - Digital motion signal  

### ESP32 #2 – Control + IoT Node
- Reads:
  - Temperature (ADC input)  
  - Motion signal  
- Controls:
  - DC motor (fan speed via PWM)  
  - Servo motor (fan direction)  
- Hosts:
  - Wi-Fi Access Point  
  - Real-time web dashboard  

---

## 🔧 Hardware Components

### Sensors
- PIR Motion Sensor  
- DHT22 Temperature Sensor  

### Controllers
- 2 × ESP32  

### Actuators
- DC Motor (fan)  
- Servo Motor (direction control)  

### Driver Circuits
- Motor driver (L298N or equivalent)  

### Others
- Power supply  
- Connecting wires  

---

## ⚙️ Working Principle

### 1. Scanning Mode
- Fan sweeps between **0° to 90°**
- Continuously checks for motion  

### 2. Detection Mode
- When motion is detected:
  - Servo locks onto detected position  
  - Scanning stops  

### 3. Adaptive Cooling
- Temperature is read and smoothed using EMA filter  
- Fan speed is adjusted automatically:
  - Low temperature → low speed  
  - High temperature → high speed  

### 4. Hold Logic
- Fan remains active for **20 seconds** after detection  
- Prevents rapid ON/OFF switching  

### 5. IoT Dashboard
- ESP32 hosts a local web server  
- Displays:
  - Temperature  
  - Motor speed  
  - Motion status  

---

## 🌐 Web Access
- SSID: SmartFan_Pro
- Password: password123
- IP Address: 192.168.4.1


---

## 📁 Code Structure

- `smart_fan_final_dc_servo.ino`
  - Main control system  
  - Servo + motor control  
  - Web server and dashboard  

- `smart_fan_final_dht22_pir.ino`
  - Sensor node  
  - Reads temperature and motion  
  - Sends signals to control ESP32  

---

## 🧪 Key Algorithms

### Temperature Smoothing (EMA)
```cpp
filteredTemp = (filteredTemp * 0.9) + (rawTemperature * 0.1);
if (filteredTemp < 25) motorSpeed = 80;
else if (filteredTemp < 30) motorSpeed = 180;
else if (filteredTemp < 32.5) motorSpeed = 200;
else motorSpeed = 255;
