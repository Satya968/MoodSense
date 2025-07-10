/*
  Combined Health Monitoring System - ESP32 Version with Web Server
  - Heart Rate Detection using MAX30105 sensor
  - Temperature reading using DS18B20 sensor  
  - GSR (Galvanic Skin Response) reading using analog sensor
  - Web server for mobile app interface
  - Enhanced mood detection with baseline and z-score analysis
  
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

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <math.h>

// Include libraries for heart rate sensor
#include <Wire.h>
#include "MAX30105.h"

// Include libraries for temperature sensor
#include <OneWire.h>
#include <DallasTemperature.h>

// WiFi credentials - CHANGE THESE TO YOUR NETWORK
const char* ssid = "iPhone 17 Pro Max";
const char* password = "mmmmmmmm";

// Web server on port 80
WebServer server(80);

// Heart rate sensor setup - CUSTOM ALGORITHM
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
const long interval = 2000;

// Debug variables
bool heartRateWorking = false;
bool tempWorking = false;
unsigned long lastHeartbeat = 0;
int consecutiveBeats = 0;

//  ENHANCED MOOD DETECTION SYSTEM 
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
const int SAMPLES_PER_45MIN = 1350; // 1350 samples per 45 minutes (one every 2 seconds)
float hr_samples[SAMPLES_PER_45MIN];
float temp_samples[SAMPLES_PER_45MIN];
float gsr_samples[SAMPLES_PER_45MIN];

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

// Current sensor values for web interface
float currentHR = 0;
float currentTemp = 0;
int currentGSR = 0;
String currentMood = "Calibrating...";
float moodProgress = 0;

// Function declarations
void initializeAdaptiveBaseline();
void performImprovedCalibration(unsigned long current_time);
void calculateAdaptiveBaseline();
void updateAdaptiveBaseline(float hr_avg, float temp_avg, float gsr_avg);
String improvedMoodDetection();
float calculateVariability(float samples[], float mean);
void printAdaptiveBaseline();
void performMoodDetection(unsigned long current_time);
void performMonitoring(unsigned long current_time);

// Web server functions
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Mood Monitor</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
            color: white;
        }
        
        .container {
            max-width: 400px;
            margin: 0 auto;
            background: rgba(255, 255, 255, 0.1);
            backdrop-filter: blur(10px);
            border-radius: 20px;
            padding: 30px;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
        }
        
        h1 {
            text-align: center;
            font-size: 2.5em;
            margin-bottom: 30px;
            text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3);
        }
        
        .mood-display {
            text-align: center;
            margin-bottom: 30px;
        }
        
        .mood-emoji {
            font-size: 5em;
            margin-bottom: 10px;
        }
        
        .mood-text {
            font-size: 1.8em;
            font-weight: bold;
            margin-bottom: 10px;
        }
        
        .progress-bar {
            width: 100%;
            height: 10px;
            background: rgba(255, 255, 255, 0.2);
            border-radius: 5px;
            overflow: hidden;
            margin-bottom: 20px;
        }
        
        .progress-fill {
            height: 100%;
            background: linear-gradient(90deg, #00ff88, #00aa55);
            transition: width 0.3s ease;
        }
        
        .sensor-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
            margin-bottom: 30px;
        }
        
        .sensor-card {
            background: rgba(255, 255, 255, 0.1);
            padding: 20px;
            border-radius: 15px;
            text-align: center;
        }
        
        .sensor-title {
            font-size: 0.9em;
            opacity: 0.8;
            margin-bottom: 10px;
        }
        
        .sensor-value {
            font-size: 1.5em;
            font-weight: bold;
        }
        
        .status {
            text-align: center;
            font-size: 1.1em;
            opacity: 0.9;
            margin-bottom: 20px;
        }
        
        .refresh-btn {
            width: 100%;
            padding: 15px;
            background: rgba(255, 255, 255, 0.2);
            border: none;
            border-radius: 10px;
            color: white;
            font-size: 1.1em;
            cursor: pointer;
            transition: background 0.3s ease;
        }
        
        .refresh-btn:hover {
            background: rgba(255, 255, 255, 0.3);
        }
        
        .offline {
            opacity: 0.5;
        }
        
        @keyframes pulse {
            0% { transform: scale(1); }
            50% { transform: scale(1.05); }
            100% { transform: scale(1); }
        }
        
        .pulse {
            animation: pulse 2s infinite;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🧠 Mood Monitor</h1>
        
        <div class="mood-display">
            <div class="mood-emoji" id="moodEmoji">🔄</div>
            <div class="mood-text" id="moodText">Calibrating...</div>
            <div class="progress-bar">
                <div class="progress-fill" id="progressFill" style="width: 0%"></div>
            </div>
        </div>
        
        <div class="sensor-grid">
            <div class="sensor-card">
                <div class="sensor-title">Heart Rate</div>
                <div class="sensor-value" id="heartRate">--</div>
                <div style="font-size: 0.8em; opacity: 0.7;">BPM</div>
            </div>
            
            <div class="sensor-card">
                <div class="sensor-title">Temperature</div>
                <div class="sensor-value" id="temperature">--</div>
                <div style="font-size: 0.8em; opacity: 0.7;">°C</div>
            </div>
            
            <div class="sensor-card">
                <div class="sensor-title">GSR</div>
                <div class="sensor-value" id="gsr">--</div>
                <div style="font-size: 0.8em; opacity: 0.7;">Raw</div>
            </div>
            
            <div class="sensor-card">
                <div class="sensor-title">Status</div>
                <div class="sensor-value" id="status">--</div>
            </div>
        </div>
        
        <div class="status" id="statusMessage">Connecting to sensors...</div>
        
        <button class="refresh-btn" onclick="refreshData()">Refresh Data</button>
    </div>

    <script>
        const moodEmojis = {
            'Happy': '😊',
            'Sad': '😢',
            'Stressed': '😰',
            'Calm': '😌',
            'Uncertain': '🤔',
            'Calibrating...': '🔄',
            'Monitoring...': '🔍'
        };
        
        function updateMoodDisplay(mood, progress) {
            document.getElementById('moodEmoji').textContent = moodEmojis[mood] || '🤔';
            document.getElementById('moodText').textContent = mood;
            document.getElementById('progressFill').style.width = progress + '%';
        }
        
        function refreshData() {
            fetch('/api/data')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('heartRate').textContent = data.heartRateWorking ? 
                        (data.heartRate || '--') : 'SENSOR ERROR';
                    document.getElementById('temperature').textContent = data.tempWorking ? 
                        (data.temperature || '--') : 'SENSOR ERROR';
                    document.getElementById('gsr').textContent = data.gsr || '--';
                    
                    // Update status based on sensor health
                    let statusText = '';
                    if (!data.heartRateWorking) {
                        statusText = 'HR Sensor Error';
                    } else if (!data.tempWorking) {
                        statusText = 'Temp Sensor Error';
                    } else if (data.fingerDetected) {
                        statusText = 'Finger OK';
                    } else {
                        statusText = 'No Finger';
                    }
                    document.getElementById('status').textContent = statusText;
                    
                    updateMoodDisplay(data.mood, data.progress);
                    
                    // Update status message
                    if (!data.heartRateWorking || !data.tempWorking) {
                        document.getElementById('statusMessage').textContent = 'Sensor connection error - check wiring';
                        document.querySelector('.container').classList.add('offline');
                    } else if (data.fingerDetected) {
                        document.getElementById('statusMessage').textContent = 'Monitoring your mood...';
                        document.querySelector('.container').classList.remove('offline');
                    } else {
                        document.getElementById('statusMessage').textContent = 'Please place finger on sensor';
                        document.querySelector('.container').classList.add('offline');
                    }
                })
                .catch(error => {
                    console.error('Error:', error);
                    document.getElementById('statusMessage').textContent = 'Connection error';
                    document.querySelector('.container').classList.add('offline');
                });
        }
        
        // Auto-refresh every 2 seconds
        setInterval(refreshData, 2000);
        
        // Initial load
        refreshData();
    </script>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html", html);
}

void handleAPI() {
  DynamicJsonDocument doc(1024);
  
  doc["heartRate"] = beatAvg;
  doc["temperature"] = currentTemp;
  doc["gsr"] = currentGSR;
  doc["mood"] = currentMood;
  doc["progress"] = moodProgress;
  doc["fingerDetected"] = fingerDetected;
  doc["heartRateWorking"] = heartRateWorking;
  doc["tempWorking"] = tempWorking;
  doc["timestamp"] = millis();
  
  String response;
  serializeJson(doc, response);
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", response);
}

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
  // calibration period of 2 minutes for baseline
  if (current_time - calibration_start_time >= 120000) { // 2 minutes
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
  
  // Update progress during calibration
  moodProgress = ((current_time - calibration_start_time) / 120000.0) * 100;
  if (moodProgress > 100) moodProgress = 100;
  
  // Collect calibration samples every 2 seconds
  if (current_time - last_sample_time >= 2000) {
     if (sample_index < 30 && fingerDetected) {
      // Only collect samples when finger is detected and readings are valid
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
  
  // Calculate mean values
  float hr_sum = 0, temp_sum = 0, gsr_sum = 0;
  
  for (int i = 0; i < sample_index; i++) {
    hr_sum += hr_samples[i];
    temp_sum += temp_samples[i];
    gsr_sum += gsr_samples[i];
  }
  
  adaptive_baseline.heart_rate_baseline = hr_sum / sample_index;
  adaptive_baseline.temperature_baseline = temp_sum / sample_index;
  adaptive_baseline.gsr_baseline = gsr_sum / sample_index;
  
  // Calculate standard deviation with minimum thresholds
  float hr_var = 0, temp_var = 0, gsr_var = 0;
  
  for (int i = 0; i < sample_index; i++) {
    hr_var += pow(hr_samples[i] - adaptive_baseline.heart_rate_baseline, 2);
    temp_var += pow(temp_samples[i] - adaptive_baseline.temperature_baseline, 2);
    gsr_var += pow(gsr_samples[i] - adaptive_baseline.gsr_baseline, 2);
  }
  
  adaptive_baseline.heart_rate_std = sqrt(hr_var / sample_index);
  adaptive_baseline.temperature_std = sqrt(temp_var / sample_index);
  adaptive_baseline.gsr_std = sqrt(gsr_var / sample_index);
  
  // Ensure minimum standard deviation to avoid over-sensitivity
  if (adaptive_baseline.heart_rate_std < 5.0) adaptive_baseline.heart_rate_std = 5.0;
  if (adaptive_baseline.temperature_std < 0.2) adaptive_baseline.temperature_std = 0.2;
  if (adaptive_baseline.gsr_std < 20.0) adaptive_baseline.gsr_std = 20.0;
  
  adaptive_baseline.measurements_count = sample_index;
  adaptive_baseline.confidence_factor = min(1.0, sample_index / 30.0);
}

void updateAdaptiveBaseline(float hr_avg, float temp_avg, float gsr_avg) {
  // Add to rolling history
  adaptive_baseline.hr_history[adaptive_baseline.history_index] = hr_avg;
  adaptive_baseline.temp_history[adaptive_baseline.history_index] = temp_avg;
  adaptive_baseline.gsr_history[adaptive_baseline.history_index] = gsr_avg;
  
  adaptive_baseline.history_index = (adaptive_baseline.history_index + 1) % 20;
  if (adaptive_baseline.history_index == 0) {
    adaptive_baseline.history_full = true;
  }
  
  // Recalculate baseline from recent history every 10 measurements
  if (adaptive_baseline.measurements_count % 10 == 0) {
    int count = adaptive_baseline.history_full ? 20 : adaptive_baseline.history_index;
    
    float hr_sum = 0, temp_sum = 0, gsr_sum = 0;
    for (int i = 0; i < count; i++) {
      hr_sum += adaptive_baseline.hr_history[i];
      temp_sum += adaptive_baseline.temp_history[i];
      gsr_sum += adaptive_baseline.gsr_history[i];
    }
    
    // Gentle adaptation - only adjust baseline by 10% toward new average
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
  // Calculate average values for the current minute
 for (int i = 0; i < SAMPLES_PER_45MIN; i++) {
    if (hr_samples[i] > 30 && hr_samples[i] < 150) {  // Valid heart rate range
      hr_avg += hr_samples[i];
      temp_avg += temp_samples[i];
      gsr_avg += gsr_samples[i];
      valid_samples++;
    }
  }
  
  if (valid_samples < 500) {
    return "Uncertain";  // Not enough valid samples
  }
  
  hr_avg /= valid_samples;
  temp_avg /= valid_samples;
  gsr_avg /= valid_samples;
  
  // Update adaptive baseline
  updateAdaptiveBaseline(hr_avg, temp_avg, gsr_avg);
  
  // Calculate z-scores using adaptive baseline
  float hr_z = (hr_avg - adaptive_baseline.heart_rate_baseline) / adaptive_baseline.heart_rate_std;
  float temp_z = (temp_avg - adaptive_baseline.temperature_baseline) / adaptive_baseline.temperature_std;
  float gsr_z = (gsr_avg - adaptive_baseline.gsr_baseline) / adaptive_baseline.gsr_std;
  
  // Calculate variability
  float hr_var = calculateVariability(hr_samples, hr_avg);
  float temp_var = calculateVariability(temp_samples, temp_avg);
  float gsr_var = calculateVariability(gsr_samples, gsr_avg);
  
  // Debug information
  Serial.println("Improved mood detection:");
  Serial.println("HR z-score: " + String(hr_z) + " (avg: " + String(hr_avg) + ")");
  Serial.println("Temp z-score: " + String(temp_z) + " (avg: " + String(temp_avg) + ")");
  Serial.println("GSR z-score: " + String(gsr_z) + " (avg: " + String(gsr_avg) + ")");
  Serial.println("Variability - HR: " + String(hr_var) + ", Temp: " + String(temp_var) + ", GSR: " + String(gsr_var));
  
  // Improved mood scoring system
  struct MoodScore {
    float calm_score = 0;
    float stressed_score = 0;
    float sad_score = 0;
    float happy_score = 0;
  } scores;
  
  // CALM detection - all values close to baseline with low variability
  if (abs(hr_z) < improved_thresholds.calm_hr_max && 
      abs(temp_z) < improved_thresholds.calm_temp_max && 
      abs(gsr_z) < improved_thresholds.calm_gsr_max) {
    scores.calm_score += 2.0;
  }
  
  if (hr_var < 6.0 && temp_var < 0.15 && gsr_var < 60.0) {
    scores.calm_score += 1.5;
  }
  
  // STRESSED detection - elevated values and/or high variability
  if (hr_z > improved_thresholds.stressed_hr_min || 
      temp_z > improved_thresholds.stressed_temp_min || 
      gsr_z > improved_thresholds.stressed_gsr_min) {
    scores.stressed_score += 2.0;
  }
  
  if (hr_var > 10.0 || temp_var > 0.25 || gsr_var > 120.0) {
    scores.stressed_score += 1.5;
  }
  
  // Multiple elevated readings indicate stress
  int elevated_count = 0;
  if (hr_z > 1.0) elevated_count++;
  if (temp_z > 0.8) elevated_count++;
  if (gsr_z > 1.0) elevated_count++;
  
  if (elevated_count >= 2) {
    scores.stressed_score += 1.0;
  }
  
  // SAD detection - moderate changes with specific patterns
  if (hr_z >= improved_thresholds.sad_hr_range_low && hr_z <= improved_thresholds.sad_hr_range_high &&
      temp_z >= improved_thresholds.sad_temp_range_low && temp_z <= improved_thresholds.sad_temp_range_high &&
      gsr_z > improved_thresholds.sad_gsr_min) {
    scores.sad_score += 2.0;
  }
  
  // Lower heart rate with elevated GSR
  if (hr_z < 0.5 && gsr_z > 0.8) {
    scores.sad_score += 1.0;
  }
  
  // HAPPY detection - positive but not extreme changes
  if (hr_z >= improved_thresholds.happy_hr_range_low && hr_z <= improved_thresholds.happy_hr_range_high &&
      temp_z >= improved_thresholds.happy_temp_range_low && temp_z <= improved_thresholds.happy_temp_range_high &&
      gsr_z >= improved_thresholds.happy_gsr_range_low && gsr_z <= improved_thresholds.happy_gsr_range_high) {
    scores.happy_score += 2.0;
  }
  
  // Moderate variability with slight elevation suggests happiness
  if (hr_var > 4.0 && hr_var < 9.0 && temp_z > 0.3 && temp_z < 1.0) {
    scores.happy_score += 1.0;
  }
  
  // Find the highest scoring mood
  float max_score = max(max(scores.calm_score, scores.stressed_score), 
                       max(scores.sad_score, scores.happy_score));
  
  Serial.println("Mood scores - Calm: " + String(scores.calm_score) + 
                 ", Stressed: " + String(scores.stressed_score) + 
                 ", Sad: " + String(scores.sad_score) + 
                 ", Happy: " + String(scores.happy_score));
  
  // Require minimum confidence for mood detection
  if (max_score < 1.5) {
    return "Calm";  // Default to calm if no strong indicators
  }
  
  if (max_score == scores.calm_score) return "Calm";
  if (max_score == scores.stressed_score) return "Stressed";
  if (max_score == scores.sad_score) return "Sad";
  if (max_score == scores.happy_score) return "Happy";
  
  return "Calm";  
}

float calculateVariability(float samples[], float mean) {
  float variance = 0;
  int valid_count = 0;
  
  for (int i = 0; i < SAMPLES_PER_45MIN; i++) {
    if (samples[i] > 0) {  // Only count valid samples
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

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  // Initialize I2C
  Wire.begin();
  delay(100);
  
  // Initialize sensors with error handling
  Serial.println("Initializing MAX30102 Heart Rate Sensor...");
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("ERROR: MAX30102 not found! Check wiring:");
    Serial.println("- SDA to GPIO 21");
    Serial.println("- SCL to GPIO 22"); 
    Serial.println("- 3.3V and GND connections");
    Serial.println("- Try different I2C address or speed");
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
  
  // Initialize temperature sensor with error handling
  Serial.println("Initializing DS18B20 Temperature Sensor...");
  sensors.begin();
  int deviceCount = sensors.getDeviceCount();
  Serial.print("Found ");
  Serial.print(deviceCount);
  Serial.println(" DS18B20 devices");
  
  if (deviceCount == 0) {
    Serial.println("ERROR: No DS18B20 found! Check:");
    Serial.println("- Data wire to GPIO 13");
    Serial.println("- 4.7kΩ pull-up resistor to 3.3V");
    Serial.println("- Power connections");
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
  
  // Initialize enhanced mood detection system
  Serial.println("Initializing Enhanced Mood Detection System...");
  Serial.println("Please remain calm for the first minute for calibration.");
  
  calibration_start_time = millis();
  current_state = CALIBRATING;
  // Initialize enhanced mood detection system
  initializeAdaptiveBaseline();
  
  // Initialize WiFi 
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.print("Connected to WiFi! IP address: ");
  Serial.println(WiFi.localIP());
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/api/data", handleAPI);
  
  server.begin();
  Serial.println("Web server started!");
  Serial.println("Open your phone browser and go to: http://" + WiFi.localIP().toString());
}

void loop() {
  // Handle web server
  server.handleClient();
  
  // Heart rate detection with error handling
  if (heartRateWorking) {
    // Check if sensor is still responding
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
      // Sensor communication failed
      fingerDetected = false;
      currentHR = 0;
      beatAvg = 0;
    }
  } else {
    // Heart rate sensor not working
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
    
    // Enhanced mood detection system
    performMoodDetection(currentMillis);
  }
  
  delay(25);
}

void performMoodDetection(unsigned long current_time) {
  if (current_state == CALIBRATING) {
  performImprovedCalibration(current_time);  
} else if (current_state == MONITORING) {
    performMonitoring(current_time);
  }
}

// Improved calibration with longer period and better validation


void performMonitoring(unsigned long current_time) {
  // Collect samples every 2 seconds for 45 minutes
  if (current_time - last_sample_time >= 2000) {
    if (sample_index < SAMPLES_PER_45MIN) {
      hr_samples[sample_index] = currentHR;
      temp_samples[sample_index] = currentTemp;
      gsr_samples[sample_index] = currentGSR;
      sample_index++;
      last_sample_time = current_time;
      
      // Update progress
      moodProgress = (sample_index / (float)SAMPLES_PER_45MIN) * 100;
      
      if (sample_index >= SAMPLES_PER_45MIN) {
        period_complete = true;
      }
    }
  }
  
  // Process mood detection after collecting 45 minutes of data
  if (period_complete) {
    String detected_mood = improvedMoodDetection();
    currentMood = detected_mood;
    moodProgress = 100;
    Serial.println("Mood detected after 45 minutes: " + detected_mood);
    
    // Reset for next 45-minute period
    sample_index = 0;
    period_complete = false;
    moodProgress = 0;
  }
}




