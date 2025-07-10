/*
  Combined Health Monitoring System - ESP32 Version with Fixed HR Algorithm
  - Heart Rate Detection using MAX30105 sensor (IMPROVED ALGORITHM)
  - Temperature reading using DS18B20 sensor  
  - GSR (Galvanic Skin Response) reading using analog sensor
  
  Hardware Connections for ESP32:
  MAX30105 Breakout to ESP32:
  -3.3V = 3.3V
  -GND = GND
  -SDA = GPIO 21 (SDA)
  -SCL = GPIO 22 (SCL)
  -INT = Not connected
  
  DS18B20 Temperature Sensor:
  -Data wire = GPIO 13 (D13)
  -VCC = 3.3V
  -GND = GND
  -REQUIRED: 4.7kΩ pull-up resistor between data and 3.3V
  
  GSR Sensor:
  -Signal = GPIO 36
  -VCC = 3.3V
  -GND = GND
*/

// Include libraries for heart rate sensor
#include <Wire.h>
#include "MAX30105.h"

// Include libraries for temperature sensor
#include <OneWire.h>
#include <DallasTemperature.h>

// Heart rate sensor setup - CUSTOM ALGORITHM (FIXED)
MAX30105 particleSensor;
const byte RATE_SIZE = 8;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute = 0;
int beatAvg = 0;

// Enhanced beat detection variables
long irValue = 0;
bool fingerDetected = false;
int beatCount = 0;

// Simplified beat detection
long lastIR = 0;
long derivative = 0;
long lastDerivative = 0;

// Adaptive threshold system
long adaptiveThreshold = 0;
long signalBaseline = 0;
bool thresholdInitialized = false;

// Smoothing filter for IR values
const int SMOOTH_SIZE = 3;
long smoothBuffer[SMOOTH_SIZE];
int smoothIndex = 0;
long smoothedIR = 0;

// Signal tracking
long signalMin = 999999;
long signalMax = 0;
long signalRange = 0;
int sampleCount = 0;

// Temperature sensor setup
#define ONE_WIRE_BUS 13  // GPIO 13 (D13) for ESP32
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// GSR sensor setup
#define GSR_PIN 36  // GPIO 36 (A0) for ESP32
int gsrValue = 0;
float gsrVoltage = 0.0;
float gsrResistance = 0.0;

// Timing variables 
unsigned long previousMillis = 0;
const long interval = 30000;  // 30000 (30 seconds)

// Debug variables
bool heartRateWorking = false;
bool tempWorking = false;
unsigned long lastHeartbeat = 0;
int consecutiveBeats = 0;

void setup()
{
  Serial.begin(115200);
  //Serial.println("=== Health Monitoring System - ESP32 Version (Fixed HR Algorithm) ===");
  delay(2000); // Give time for serial to initialize
  
  // Initialize heart rate sensor with improved configuration
  //Serial.println("Initializing Heart Rate Sensor...");
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
  {
    //Serial.println("ERROR: MAX30105 not found! Check wiring/power.");
    heartRateWorking = false;
  }
  else
  {
    //Serial.println("MAX30105 found!");
    
    // Enhanced configuration for better sensitivity
    particleSensor.setup();
    particleSensor.setPulseAmplitudeRed(0x0A);
    particleSensor.setPulseAmplitudeIR(0x1F);
    
    heartRateWorking = true;
    //Serial.println("Heart rate sensor configured with high sensitivity.");
  }
  
  // Initialize the rates array
  for (byte i = 0; i < RATE_SIZE; i++) {
    rates[i] = 0;
  }
  
  // Initialize smoothing buffer
  for (int i = 0; i < SMOOTH_SIZE; i++) {
    smoothBuffer[i] = 0;
  }
  
  // Initialize temperature sensor
  //Serial.println("Initializing Temperature Sensor...");
  //Serial.println("Using GPIO 13 (D13) for DS18B20 data line");
  sensors.begin();
  
  //Serial.print("Searching for DS18B20 sensors... Found: ");
  int deviceCount = sensors.getDeviceCount();
  //Serial.print(deviceCount);
  //Serial.println(" devices");
  
  if (deviceCount == 0) {
    //Serial.println("ERROR: No DS18B20 sensors found!");
    //Serial.println("Check: 1) Wiring to GPIO 13, 2) 4.7kΩ pull-up resistor to 3.3V, 3) Power supply");
    tempWorking = false;
  } else {
    tempWorking = true;
    //Serial.println("DS18B20 sensor found!");
    
    // Set resolution for faster readings
    sensors.setResolution(12);
    
    if (sensors.isParasitePowerMode()) {
      //Serial.println("WARNING: Sensor in parasite power mode - may be unreliable");
    }
  }
  
  // Initialize GSR sensor
  pinMode(GSR_PIN, INPUT);
  //Serial.println("GSR sensor initialized on GPIO 36 (A0)");
  
  //Serial.println("=== System Status ===");
  //Serial.print("Heart Rate Sensor: ");
  //Serial.println(heartRateWorking ? "WORKING" : "FAILED");
  //Serial.print("Temperature Sensor: ");
  //Serial.println(tempWorking ? "WORKING" : "FAILED");
  //Serial.println("GSR Sensor: WORKING");
  

}
void loop() {
  // Toggle for extra debug messages
  bool debugMode = false;

  // Heart rate detection with FIXED CUSTOM ALGORITHM
  if (heartRateWorking) {
    irValue = particleSensor.getIR();

    // Light smoothing
    smoothBuffer[smoothIndex] = irValue;
    smoothIndex = (smoothIndex + 1) % SMOOTH_SIZE;

    smoothedIR = 0;
    for (int i = 0; i < SMOOTH_SIZE; i++) {
      smoothedIR += smoothBuffer[i];
    }
    smoothedIR /= SMOOTH_SIZE;

    // Check finger presence
    fingerDetected = (smoothedIR > 20000);

    if (!fingerDetected) {
      if (millis() % 5000 == 0 && debugMode) {
        Serial.println("HEART RATE: No finger detected (IR < 20000)");
      }

      beatCount = 0;
      sampleCount = 0;
      signalMin = 999999;
      signalMax = 0;
      beatsPerMinute = 0;
      beatAvg = 0;
      thresholdInitialized = false;
    } else {
      sampleCount++;
      if (smoothedIR < signalMin) signalMin = smoothedIR;
      if (smoothedIR > signalMax) signalMax = smoothedIR;
      signalRange = signalMax - signalMin;

      if (sampleCount == 30) {
        signalBaseline = (signalMin + signalMax) / 2;
        adaptiveThreshold = signalBaseline + (signalRange * 0.15);
        thresholdInitialized = true;
        if (debugMode) Serial.println("*** THRESHOLD INITIALIZED ***");
      }

      if (sampleCount > 30 && sampleCount % 15 == 0) {
        signalBaseline = (signalMin + signalMax) / 2;
        adaptiveThreshold = signalBaseline + (signalRange * 0.15);
      }

      if (sampleCount > 5 && thresholdInitialized) {
        derivative = smoothedIR - lastIR;
        bool peakDetected = (lastDerivative > 0 && derivative <= 0 && smoothedIR > adaptiveThreshold);
        bool strongSignal = (signalRange > 300);

        if (peakDetected && strongSignal) {
          unsigned long now = millis();
          if (now - lastBeat > 400) {
            if (lastBeat > 0) {
              long delta = now - lastBeat;
              if (delta > 400 && delta < 2000) {
                beatCount++;
                beatsPerMinute = 60000.0 / delta;

                if (debugMode) {
                  Serial.print("*** BEAT DETECTED *** #");
                  Serial.print(beatCount);
                  Serial.print(" Delta: ");
                  Serial.print(delta);
                  Serial.print("ms, BPM: ");
                  Serial.println(beatsPerMinute);
                }

                if (beatsPerMinute >= 30 && beatsPerMinute <= 150) {
                  rates[rateSpot++] = (byte)beatsPerMinute;
                  rateSpot %= RATE_SIZE;

                  long total = 0;
                  int count = 0;
                  for (byte x = 0; x < RATE_SIZE; x++) {
                    if (rates[x] > 0) {
                      total += rates[x];
                      count++;
                    }
                  }

                  if (count > 0) {
                    beatAvg = total / count;
                  }
                }
              }
            }
            lastBeat = now;
          }
        }

        lastDerivative = derivative;
      }

      lastIR = smoothedIR;
    }
  }

  // Read temperature and GSR at regular intervals - EVERY 30 SECONDS
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float tempC = 0.0;
    bool tempValid = false;
    if (tempWorking) {
      sensors.requestTemperatures();
      delay(100);
      tempC = sensors.getTempCByIndex(0);
      if (tempC != DEVICE_DISCONNECTED_C && tempC != 85.0) {
        tempValid = true;
      }
    }

    gsrValue = analogRead(GSR_PIN);
    gsrVoltage = gsrValue * (3.3 / 4095.0);
    if (gsrVoltage > 0) {
      gsrResistance = (3.3 - gsrVoltage) * 10000.0 / gsrVoltage;
    } else {
      gsrResistance = 0;
    }

    // CSV-Formatted Output -  EVERY 30 SECONDS
    Serial.print(currentMillis / 1000); Serial.print(",");     // Time (s)
    Serial.print(beatAvg); Serial.print(",");                 // Heart Rate Avg
    if (tempValid)
      Serial.print(tempC, 2);  // Temperature (°C)
    else
      Serial.print("ERROR");
    Serial.print(",");
    Serial.println(gsrValue);                                // GSR raw

    // Optional: Add header only once
    // Serial.println("Time,HR,TEMP,GSR");
  }

  delay(25); // For smoother HR signal sampling
}
