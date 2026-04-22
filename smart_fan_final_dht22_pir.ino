#include <DHT.h>

#define PIR_PIN 14
#define PIR_OUT 19

#define DHT_PIN 4
#define DHTTYPE DHT22

#define TEMP_OUT 25   // DAC pin

DHT dht(DHT_PIN, DHTTYPE);

void setup() {
  Serial.begin(115200);

  pinMode(PIR_PIN, INPUT);
  pinMode(PIR_OUT, OUTPUT);

  dht.begin();
}

void loop() {

  // 🔴 Read PIR
  int pirState = digitalRead(PIR_PIN);
  digitalWrite(PIR_OUT, pirState);

  // 🌡 Read Temperature
  float temp = dht.readTemperature();
  if (isnan(temp)) temp = 25;

  // Convert temp (20.0°C - 40.0°C) → DAC Value (0 - 255) using float for precision
  float dacPrecise = (temp - 20.0) * 255.0 / 20.0;
  int dacValue = (int)constrain(dacPrecise, 0, 255);
  
  dacWrite(TEMP_OUT, dacValue);

  // 🖥 SERIAL OUTPUT
  Serial.print("PIR: ");
  Serial.print(pirState == HIGH ? "MOTION" : "NO MOTION");

  Serial.print(" | Temp: ");
  Serial.print(temp);
  Serial.print(" °C");

  Serial.print(" | DAC Sent: ");
  Serial.println(dacValue);

  delay(500);
}
