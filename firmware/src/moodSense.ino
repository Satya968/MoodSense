#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include "MAX30105.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>
#include <ArduinoJson.h>
// BLE Setup
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Heart rate sensor setup
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

#define BATTERY_PIN 33  // GPIO39 (VN port) for charging %
int currentBattery = 0;
// Timing variables
unsigned long previousMillis = 0;
const long interval = 1000;

// Debug variables
bool heartRateWorking = false;
bool tempWorking = false;
unsigned long lastHeartbeat = 0;
int consecutiveBeats = 0;

// ENHANCED MOOD DETECTION SYSTEM 
// Calibration and baseline variables
struct AdaptiveBaseline {
  float heart_rate_baseline;
  float temperature_baseline;
  float gsr_baseline;
  float heart_rate_std;
  float temperature_std;
  float gsr_std;
  
  // Rolling baseline for adaptation
  float hr_history[20];  // Store last 20 measurements
  float temp_history[20];
  float gsr_history[20];
  int history_index;
  bool history_full;
  
  // Confidence tracking
  int measurements_count;
  float confidence_factor;
};

AdaptiveBaseline adaptive_baseline;

// Data collection arrays for 1-minute windows
const int SAMPLES_PER_1MIN = 60; // 60 samples per 1 minute (one every 1 second)
float hr_samples[SAMPLES_PER_1MIN];
float temp_samples[SAMPLES_PER_1MIN];
float gsr_samples[SAMPLES_PER_1MIN];

struct ImprovedMoodThresholds {
  // More realistic z-score thresholds
  float calm_hr_max = 0.8;      
  float calm_temp_max = 0.6;    
  float calm_gsr_max = 0.7;     
  
  float stressed_hr_min = 1.8;   
  float stressed_temp_min = 1.2; 
  float stressed_gsr_min = 1.5;  
  
  float sad_hr_range_low = -0.8;  // Wider range
  float sad_hr_range_high = 1.2;
  float sad_temp_range_low = -0.5;
  float sad_temp_range_high = 0.8;
  float sad_gsr_min = 0.9;       
  
  float happy_hr_range_low = 0.4;
  float happy_hr_range_high = 1.6;
  float happy_temp_range_low = 0.1;
  float happy_temp_range_high = 1.0;
  float happy_gsr_range_low = -0.4;
  float happy_gsr_range_high = 0.8;
};

ImprovedMoodThresholds improved_thresholds;

// System state
enum SystemState {
  CALIBRATING,
  MONITORING
};

SystemState current_state = CALIBRATING;
unsigned long calibration_start_time = 0;
unsigned long last_sample_time = 0;
int sample_index = 0;
bool period_complete = false;

// Current sensor values
float currentHR = 0;
float currentTemp = 0;
int currentGSR = 0;
String currentMood = "Calibrating...";
float moodProgress = 0;

// BLE Server Callbacks
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) { 
        deviceConnected = true; 
        Serial.println("Device connected");
    };
    void onDisconnect(BLEServer* pServer) { 
        deviceConnected = false; 
        Serial.println("Device disconnected");
        // Restart advertising
        BLEDevice::startAdvertising();
    }
};

void initializeBLE() {
    Serial.println("Initializing BLE...");
    
    // Initialize BLE Device
    BLEDevice::init("Health_Monitor_BLE");
    
    // Create BLE Server
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    
    // Create BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);
    
    // Create BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID, 
        BLECharacteristic::PROPERTY_READ | 
        BLECharacteristic::PROPERTY_WRITE | 
        BLECharacteristic::PROPERTY_NOTIFY
    );
    
    // Add descriptor for notifications
    pCharacteristic->addDescriptor(new BLE2902());
    
    // Start the service
    pService->start();
    
    // Start advertising
    BLEDevice::getAdvertising()->addServiceUUID(SERVICE_UUID);
    BLEDevice::startAdvertising();
    
    Serial.println("BLE initialization complete!");
}
int getBatteryPercentage(float voltage) {
  float minVoltage = 3.2;  // Empty
  float maxVoltage = 4.1;  // Full
  
  if (voltage <= minVoltage) return 0;
  if (voltage >= maxVoltage) return 100;
  
  // Calculate percentage
  int percentage = (voltage - minVoltage) / (maxVoltage - minVoltage) * 100;
  return percentage;
}

// Function declarations for mood detection
void initializeAdaptiveBaseline();
void performImprovedCalibration(unsigned long current_time);
void calculateAdaptiveBaseline();
void updateAdaptiveBaseline(float hr_avg, float temp_avg, float gsr_avg);
String improvedMoodDetection();
float calculateVariability(float samples[], float mean);
void printAdaptiveBaseline();
void performMoodDetection(unsigned long current_time);
void performMonitoring(unsigned long current_time);

void initializeAdaptiveBaseline() {
  adaptive_baseline.heart_rate_baseline = 0;
  adaptive_baseline.temperature_baseline = 0;
  adaptive_baseline.gsr_baseline = 0;
  adaptive_baseline.heart_rate_std = 1.0;
  adaptive_baseline.temperature_std = 0.1;
  adaptive_baseline.gsr_std = 10.0;
  
  adaptive_baseline.history_index = 0;
  adaptive_baseline.history_full = false;
  adaptive_baseline.measurements_count = 0;
  adaptive_baseline.confidence_factor = 0.0;
  
  // Initialize history arrays
  for (int i = 0; i < 20; i++) {
    adaptive_baseline.hr_history[i] = 0;
    adaptive_baseline.temp_history[i] = 0;
    adaptive_baseline.gsr_history[i] = 0;
  }
}

void performImprovedCalibration(unsigned long current_time) {
  if (current_time - calibration_start_time >= 60000) { // 1 minute
    calculateAdaptiveBaseline();
    current_state = MONITORING;
    sample_index = 0;
    last_sample_time = current_time;
    Serial.println("Extended calibration complete. Starting mood monitoring...");
    printAdaptiveBaseline();
    currentMood = "Monitoring...";
    moodProgress = 0;
    return;
  }
  
  moodProgress = ((current_time - calibration_start_time) / 60000.0) * 100;
  if (moodProgress > 100) moodProgress = 100;
  
  if (current_time - last_sample_time >= 1000) {
     if (sample_index < 30 && fingerDetected) {
      if (currentHR > 30 && currentHR < 150 && currentTemp > 30 && currentTemp < 42) {
        hr_samples[sample_index] = currentHR;
        temp_samples[sample_index] = currentTemp;
        gsr_samples[sample_index] = currentGSR;
        sample_index++;
        Serial.println("Calibration sample " + String(sample_index) + "/30 collected");
      }
      last_sample_time = current_time;
    }
  }
}

void calculateAdaptiveBaseline() {
  if (sample_index < 10) {
    Serial.println("Insufficient calibration samples. Using default baseline.");
    adaptive_baseline.heart_rate_baseline = 75.0;
    adaptive_baseline.temperature_baseline = 36.5;
    adaptive_baseline.gsr_baseline = 500.0;
    return;
  }
  
  float hr_sum = 0, temp_sum = 0, gsr_sum = 0;
  
  for (int i = 0; i < sample_index; i++) {
    hr_sum += hr_samples[i];
    temp_sum += temp_samples[i];
    gsr_sum += gsr_samples[i];
  }
  
  adaptive_baseline.heart_rate_baseline = hr_sum / sample_index;
  adaptive_baseline.temperature_baseline = temp_sum / sample_index;
  adaptive_baseline.gsr_baseline = gsr_sum / sample_index;
  
  float hr_var = 0, temp_var = 0, gsr_var = 0;
  
  for (int i = 0; i < sample_index; i++) {
    hr_var += pow(hr_samples[i] - adaptive_baseline.heart_rate_baseline, 2);
    temp_var += pow(temp_samples[i] - adaptive_baseline.temperature_baseline, 2);
    gsr_var += pow(gsr_samples[i] - adaptive_baseline.gsr_baseline, 2);
  }
  
  adaptive_baseline.heart_rate_std = sqrt(hr_var / sample_index);
  adaptive_baseline.temperature_std = sqrt(temp_var / sample_index);
  adaptive_baseline.gsr_std = sqrt(gsr_var / sample_index);
  
  if (adaptive_baseline.heart_rate_std < 5.0) adaptive_baseline.heart_rate_std = 5.0;
  if (adaptive_baseline.temperature_std < 0.2) adaptive_baseline.temperature_std = 0.2;
  if (adaptive_baseline.gsr_std < 20.0) adaptive_baseline.gsr_std = 20.0;
  
  adaptive_baseline.measurements_count = sample_index;
  adaptive_baseline.confidence_factor = min(1.0, sample_index / 30.0);
}

void updateAdaptiveBaseline(float hr_avg, float temp_avg, float gsr_avg) {
  adaptive_baseline.hr_history[adaptive_baseline.history_index] = hr_avg;
  adaptive_baseline.temp_history[adaptive_baseline.history_index] = temp_avg;
  adaptive_baseline.gsr_history[adaptive_baseline.history_index] = gsr_avg;
  
  adaptive_baseline.history_index = (adaptive_baseline.history_index + 1) % 20;
  if (adaptive_baseline.history_index == 0) {
    adaptive_baseline.history_full = true;
  }
  
  if (adaptive_baseline.measurements_count % 10 == 0) {
    int count = adaptive_baseline.history_full ? 20 : adaptive_baseline.history_index;
    
    float hr_sum = 0, temp_sum = 0, gsr_sum = 0;
    for (int i = 0; i < count; i++) {
      hr_sum += adaptive_baseline.hr_history[i];
      temp_sum += adaptive_baseline.temp_history[i];
      gsr_sum += adaptive_baseline.gsr_history[i];
    }
    
    float new_hr_baseline = hr_sum / count;
    float new_temp_baseline = temp_sum / count;
    float new_gsr_baseline = gsr_sum / count;
    
    adaptive_baseline.heart_rate_baseline = adaptive_baseline.heart_rate_baseline * 0.9 + new_hr_baseline * 0.1;
    adaptive_baseline.temperature_baseline = adaptive_baseline.temperature_baseline * 0.9 + new_temp_baseline * 0.1;
    adaptive_baseline.gsr_baseline = adaptive_baseline.gsr_baseline * 0.9 + new_gsr_baseline * 0.1;
    
    Serial.println("Baseline adapted: HR=" + String(adaptive_baseline.heart_rate_baseline) + 
                   ", Temp=" + String(adaptive_baseline.temperature_baseline) + 
                   ", GSR=" + String(adaptive_baseline.gsr_baseline));
  }
  
  adaptive_baseline.measurements_count++;
}

String improvedMoodDetection() {
  float hr_avg = 0, temp_avg = 0, gsr_avg = 0;
  int valid_samples = 0;
  
  for (int i = 0; i < SAMPLES_PER_1MIN; i++) {
    if (hr_samples[i] > 30 && hr_samples[i] < 150) {
      hr_avg += hr_samples[i];
      temp_avg += temp_samples[i];
      gsr_avg += gsr_samples[i];
      valid_samples++;
    }
  }
  
  // Changed: If insufficient samples, return "Stressed" instead of "Uncertain"
  if (valid_samples < 5) {
    return "Stressed";
  }
  
  hr_avg /= valid_samples;
  temp_avg /= valid_samples;
  gsr_avg /= valid_samples;
  
  updateAdaptiveBaseline(hr_avg, temp_avg, gsr_avg);
  
  float hr_z = (hr_avg - adaptive_baseline.heart_rate_baseline) / adaptive_baseline.heart_rate_std;
  float temp_z = (temp_avg - adaptive_baseline.temperature_baseline) / adaptive_baseline.temperature_std;
  float gsr_z = (gsr_avg - adaptive_baseline.gsr_baseline) / adaptive_baseline.gsr_std;
  
  float hr_var = calculateVariability(hr_samples, hr_avg);
  float temp_var = calculateVariability(temp_samples, temp_avg);
  float gsr_var = calculateVariability(gsr_samples, gsr_avg);
  
  Serial.println("Improved mood detection:");
  Serial.println("HR z-score: " + String(hr_z) + " (avg: " + String(hr_avg) + ")");
  Serial.println("Temp z-score: " + String(temp_z) + " (avg: " + String(temp_avg) + ")");
  Serial.println("GSR z-score: " + String(gsr_z) + " (avg: " + String(gsr_avg) + ")");
  Serial.println("Variability - HR: " + String(hr_var) + ", Temp: " + String(temp_var) + ", GSR: " + String(gsr_var));
  
  struct MoodScore {
    float calm_score = 0;
    float stressed_score = 0;
    float sad_score = 0;
    float happy_score = 0;
  } scores;
  
  if (abs(hr_z) < improved_thresholds.calm_hr_max && 
      abs(temp_z) < improved_thresholds.calm_temp_max && 
      abs(gsr_z) < improved_thresholds.calm_gsr_max) {
    scores.calm_score += 2.0;
  }
  
  if (hr_var < 6.0 && temp_var < 0.15 && gsr_var < 60.0) {
    scores.calm_score += 1.5;
  }
  
  if (hr_z > improved_thresholds.stressed_hr_min || 
      temp_z > improved_thresholds.stressed_temp_min || 
      gsr_z > improved_thresholds.stressed_gsr_min) {
    scores.stressed_score += 2.0;
  }
  
  if (hr_var > 10.0 || temp_var > 0.25 || gsr_var > 120.0) {
    scores.stressed_score += 1.5;
  }
  
  int elevated_count = 0;
  if (hr_z > 1.0) elevated_count++;
  if (temp_z > 0.8) elevated_count++;
  if (gsr_z > 1.0) elevated_count++;
  
  if (elevated_count >= 2) {
    scores.stressed_score += 1.0;
  }
  
  if (hr_z >= improved_thresholds.sad_hr_range_low && hr_z <= improved_thresholds.sad_hr_range_high &&
      temp_z >= improved_thresholds.sad_temp_range_low && temp_z <= improved_thresholds.sad_temp_range_high &&
      gsr_z > improved_thresholds.sad_gsr_min) {
    scores.sad_score += 2.0;
  }
  
  if (hr_z < 0.5 && gsr_z > 0.8) {
    scores.sad_score += 1.0;
  }
  
  if (hr_z >= improved_thresholds.happy_hr_range_low && hr_z <= improved_thresholds.happy_hr_range_high &&
      temp_z >= improved_thresholds.happy_temp_range_low && temp_z <= improved_thresholds.happy_temp_range_high &&
      gsr_z >= improved_thresholds.happy_gsr_range_low && gsr_z <= improved_thresholds.happy_gsr_range_high) {
    scores.happy_score += 2.0;
  }
  
  if (hr_var > 4.0 && hr_var < 9.0 && temp_z > 0.3 && temp_z < 1.0) {
    scores.happy_score += 1.0;
  }
  
  float max_score = max(max(scores.calm_score, scores.stressed_score), 
                       max(scores.sad_score, scores.happy_score));
  
  Serial.println("Mood scores - Calm: " + String(scores.calm_score) + 
                 ", Stressed: " + String(scores.stressed_score) + 
                 ", Sad: " + String(scores.sad_score) + 
                 ", Happy: " + String(scores.happy_score));
  
  // Changed: If no clear mood detected, default to "Stressed" instead of "Calm"
  if (max_score < 1.5) {
    return "Stressed";
  }
  
  if (max_score == scores.calm_score) return "Calm";
  if (max_score == scores.stressed_score) return "Stressed";
  if (max_score == scores.sad_score) return "Sad";
  if (max_score == scores.happy_score) return "Happy";
  
  // Changed: Final fallback to "Stressed" instead of "Calm"
  return "Stressed";  
}

float calculateVariability(float samples[], float mean) {
  float variance = 0;
  int valid_count = 0;
  
  for (int i = 0; i < SAMPLES_PER_1MIN; i++) {
    if (samples[i] > 0) {
      variance += pow(samples[i] - mean, 2);
      valid_count++;
    }
  }

  if (valid_count > 0) {
    return sqrt(variance / valid_count);
  }
  return 0;
}

void printAdaptiveBaseline() {
  Serial.println("Adaptive baseline established:");
  Serial.println("Heart Rate: " + String(adaptive_baseline.heart_rate_baseline) + " ± " + String(adaptive_baseline.heart_rate_std));
  Serial.println("Temperature: " + String(adaptive_baseline.temperature_baseline) + " ± " + String(adaptive_baseline.temperature_std));
  Serial.println("GSR: " + String(adaptive_baseline.gsr_baseline) + " ± " + String(adaptive_baseline.gsr_std));
  Serial.println("Confidence: " + String(adaptive_baseline.confidence_factor * 100) + "%");
}

void performMoodDetection(unsigned long current_time) {
  if (current_state == CALIBRATING) {
    performImprovedCalibration(current_time);  
  } else if (current_state == MONITORING) {
    performMonitoring(current_time);
  }
}

void performMonitoring(unsigned long current_time) {
  if (current_time - last_sample_time >= 1000) {
    if (sample_index < SAMPLES_PER_1MIN) {
      hr_samples[sample_index] = currentHR;
      temp_samples[sample_index] = currentTemp;
      gsr_samples[sample_index] = currentGSR;
      sample_index++;
      last_sample_time = current_time;
      
      moodProgress = (sample_index / (float)SAMPLES_PER_1MIN) * 100;
      
      if (sample_index >= SAMPLES_PER_1MIN) {
        period_complete = true;
      }
    }
  }
  
  if (period_complete) {
    String detected_mood = improvedMoodDetection();
    currentMood = detected_mood;
    moodProgress = 100;
    Serial.println("Mood detected after 1 minute: " + detected_mood);
    
    sample_index = 0;
    period_complete = false;
    moodProgress = 0;
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  // Initialize BLE
  initializeBLE();
  
  // Initialize I2C
  Wire.begin();
  delay(100);
  
  // Initialize heart rate sensor
  Serial.println("Initializing MAX30102 Heart Rate Sensor...");
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("ERROR: MAX30102 not found!");
    heartRateWorking = false;
  } else {
    Serial.println("MAX30105 found and initialized!");
    particleSensor.setup();
    particleSensor.setPulseAmplitudeRed(0x0A);
    particleSensor.setPulseAmplitudeIR(0x1F);
    heartRateWorking = true;
  }
  
  for (byte i = 0; i < RATE_SIZE; i++) {
    rates[i] = 0;
  }
  
  for (int i = 0; i < SMOOTH_SIZE; i++) {
    smoothBuffer[i] = 0;
  }
  
  // Initialize temperature sensor
  Serial.println("Initializing DS18B20 Temperature Sensor...");
  sensors.begin();
  int deviceCount = sensors.getDeviceCount();
  Serial.print("Found ");
  Serial.print(deviceCount);
  Serial.println(" DS18B20 devices");
  
  if (deviceCount == 0) {
    Serial.println("ERROR: No DS18B20 found!");
    tempWorking = false;
  } else {
    tempWorking = true;
    sensors.setResolution(12);
    Serial.println("DS18B20 initialized!");
  }
  
  pinMode(GSR_PIN, INPUT);
  Serial.println("GSR sensor initialized on GPIO 36");
  
  // Print sensor status
  Serial.println("=== SENSOR STATUS ===");
  Serial.print("Heart Rate (MAX30105): ");
  Serial.println(heartRateWorking ? "OK" : "FAILED");
  Serial.print("Temperature (DS18B20): ");
  Serial.println(tempWorking ? "OK" : "FAILED");
  Serial.println("GSR: OK");
  Serial.println("====================");
  pinMode(BATTERY_PIN, INPUT);
  Serial.println("Battery monitor initialized on GPIO 39");
  // Initialize enhanced mood detection system
  Serial.println("Initializing Enhanced Mood Detection System...");
  Serial.println("Please remain calm for the first minute for calibration.");
  
  calibration_start_time = millis();
  current_state = CALIBRATING;
  initializeAdaptiveBaseline();
}

void loop() {
  // Heart rate detection
  if (heartRateWorking) {
    if (particleSensor.safeCheck(250)) {
      irValue = particleSensor.getIR();
      
      smoothBuffer[smoothIndex] = irValue;
      smoothIndex = (smoothIndex + 1) % SMOOTH_SIZE;
      
      smoothedIR = 0;
      for (int i = 0; i < SMOOTH_SIZE; i++) {
        smoothedIR += smoothBuffer[i];
      }
      smoothedIR /= SMOOTH_SIZE;
      
      fingerDetected = (smoothedIR > 20000);
      
      if (!fingerDetected) {
        beatCount = 0;
        sampleCount = 0;
        signalMin = 999999;
        signalMax = 0;
        beatsPerMinute = 0;
        beatAvg = 0;
        thresholdInitialized = false;
        currentHR = 0;
      } else {
        sampleCount++;
        if (smoothedIR < signalMin) signalMin = smoothedIR;
        if (smoothedIR > signalMax) signalMax = smoothedIR;
        signalRange = signalMax - signalMin;
        
        if (sampleCount == 30) {
          signalBaseline = (signalMin + signalMax) / 2;
          adaptiveThreshold = signalBaseline + (signalRange * 0.15);
          thresholdInitialized = true;
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
                      currentHR = beatAvg;
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
    } else {
      fingerDetected = false;
      currentHR = 0;
      beatAvg = 0;
    }
  } else {
    fingerDetected = false;
    currentHR = 0;
    beatAvg = 0;
  }
  
  // Read temperature and GSR at regular intervals
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
        currentTemp = tempC;
      }
    }
    
    gsrValue = analogRead(GSR_PIN);
    currentGSR = gsrValue;
    
    int rawValue = analogRead(BATTERY_PIN);
    float batteryVoltage = (rawValue / 4095.0) * 3.3 * 2.0;  // Voltage divider factor of 2
    currentBattery = getBatteryPercentage(batteryVoltage);

    // Enhanced mood detection system
    performMoodDetection(currentMillis);

    // Prepare JSON data for BLE transmission
    DynamicJsonDocument doc(256);
    doc["hr"] = currentHR;
    doc["temp"] = currentTemp;
    doc["gsr"] = currentGSR;
    doc["mood"] = currentMood;
    doc["progress"] = moodProgress;
    doc["finger"] = fingerDetected;
    doc["battery"] = currentBattery;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Send data via BLE
    if (deviceConnected) {
      pCharacteristic->setValue(jsonString.c_str());
      pCharacteristic->notify();
      Serial.println("Sent BLE data: " + jsonString);
    }
  }
  
  delay(25);
}
