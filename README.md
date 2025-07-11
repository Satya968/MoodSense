# MoodSense
A Continuous Mood &amp; Mental Health Logger for Remote Patient Monitoring
## Overview

This project implements a real-time mood detection system using physiological sensors including GSR (Galvanic Skin Response), heart rate monitoring via MAX30102, and temperature sensing with DS18B20. The system establishes personalized baselines through meditation sessions and uses advanced signal processing algorithms to detect emotional states(Happy😊, Calm😌, Sad😢, Stressed😰).

## Features

- **Multi-sensor Integration**: GSR, heart rate (MAX30102), and temperature (DS18B20) sensors
- **Personalized Baseline Calibration**: Individual baseline establishment through guided meditation
- **Real-time Mood Detection**: Continuous monitoring with 45-minute detection intervals
- **Web-based Interface**: Browser-accessible dashboard for mood tracking
- **Data Logging**: Comprehensive sensor data and mood state logging
- **Accuracy Validation**: Empirical validation against reference devices

## Hardware Components

- ESP32 microcontroller
- MAX30102 heart rate and SpO2 sensor
- DS18B20 temperature sensor
- GSR sensor module
- Supporting electronics and connections

## System Architecture

### Sensor Layer
- **GSR Sensor**: Measures skin conductance variations
- **MAX30102**: Provides heart rate and blood oxygen saturation
- **DS18B20**: Monitors body temperature fluctuations

### Processing Layer
- **Baseline Calibration**: 1-2 minute meditation-based calibration
- **Signal Processing**: Z-score computation and threshold-based analysis
- **Mood Classification**: Multi-parameter decision algorithm

### Interface Layer
- **Web Dashboard**: Real-time mood visualization
- **Data Export**: CSV format for further analysis
- **Network Communication**: Wi-Fi enabled data transmission

## Algorithm Details

### Baseline Establishment
1. User performs 1-2 minute guided meditation
2. System records stable physiological readings
3. Baseline parameters calculated for personalized thresholds

### Mood Detection Algorithm
- **Data Acquisition**: Sensor readings every 30 seconds
- **Preprocessing**: Signal filtering and noise reduction
- **Feature Extraction**: Statistical measures and differential analysis
- **Classification**: Z-score based threshold comparison
- **Output**: Mood state classification every 45 minutes

## Validation and Testing

### Accuracy Assessment
- **MAX30102**: Validated against commercial pulse oximeter over 1-hour duration
- **DS18B20**: Compared with IR temperature gun measurements
- **Sampling Rate**: 30-second intervals for comprehensive comparison

### Drift Analysis
- **Duration**: 15-hour continuous monitoring
- **All Sensors**: Comprehensive drift characterization
- **Stability**: Long-term performance validation

### Real-world Testing
- **Duration**: 12-hour continuous operation
- **Mood Logging**: Manual mood verification every 45 minutes
- **Accuracy**: Achieved `50`% accuracy against ground truth

## Installation and Setup

### Hardware Setup
1. Connect sensors to ESP32 according to wiring diagram
2. Ensure proper power supply and grounding
3. Verify sensor connectivity and calibration
4. Test individual sensors by running example codes from their respective libraries to ensure proper functionality

### Software Installation

#### For Main MoodSense Application
1. Navigate to `firmware/src/` directory
2. Open [`moodSense.ino`](firmware/src/moodSense.ino) in Arduino IDE
3. Download and install the following Arduino libraries:
   - WiFi
   - WebServer
   - ArduinoJson
   - Wire
   - MAX30105
   - OneWire
   - DallasTemperature
   - math
4. Upload the code to ESP32

#### For Data Logging
1. Navigate to `firmware/data_logging/` directory
2. Open [`sensor_logger.ino`](firmware/data_logging/sensor_logger.ino) in Arduino IDE
3. Download and install the following Arduino libraries:
   - Wire
   - MAX30105
   - OneWire
   - DallasTemperature
4. Upload the code to ESP32

#### For CSV Data Generation
1. Navigate to `firmware/data_logging/` directory
2. Install Python dependencies for [`csv_generator.py`](firmware/data_logging/csv_generator.py):
   - serial
   - csv
   - os
   - datetime
3. Run the Python script to generate CSV files from sensor data

### Network Configuration
1. Configure Wi-Fi credentials in ESP32 code
2. Deploy web interface to local network
3. Access dashboard via provided IP address in Serial Monitor

## Usage

### Initial Calibration
1. Power on the system
2. Follow meditation guide for baseline establishment
3. Check the web dashboard for calibration status

### Continuous Monitoring
1. Wear sensors as instructed
2. Access web dashboard for real-time monitoring
3. View mood detection results every 45 minutes

## Data Output

### Sensor Data
(by using [`sensor_logger.ino`](firmware/data_logging/sensor_logger.ino),[`csv_generator.py`](firmware/data_logging/csv_generator.py))
- **Format**: CSV with timestamps
- **Frequency**: 2 readings per minute
- **Parameters**: GSR, heart rate, temperature, current consumption

### Mood Detection
(by using [`moodSense.ino`](firmware/src/moodSense.ino))
- **Frequency**: Every 45 minutes
- **Output**: Classified mood state with confidence metrics
- **Logging**: Comprehensive mood history with timestamps

## Technical Specifications

### Sensor Specifications
- **GSR**: any value between 0-4095
- **MAX30102**: Heart rate ±3 BPM
- **DS18B20**: ±0.5°C accuracy

### Performance Metrics
- **Power Consumption**: 1.28A at 3.3V input voltage(5v was also applied for some time)
- **Continuous Operation**: 12+ hours validated

## Development Challenges and Solutions

### GSR Sensor Pressure Dependency
- **Problem**: GSR sensor readings were highly dependent on the pressure applied to the sensor, causing inconsistent and unreliable measurements
- **Solution**: Implemented differential analysis using baseline-established values to eliminate pressure variations and provide more stable readings

### Heart Rate Sensor Accuracy Issues
- **Problem**: Using SparkFun example code for MAX30102 heart rate sensor resulted in consistently high readings (mostly >100 BPM) that were not accurate
- **Solution**: Developed custom algorithm based on heart rate plotter example code to achieve better accuracy and more realistic heart rate measurements

### Mobile App Development Constraints
- **Problem**: Team lacked iOS app development experience and faced time constraints for developing a native mobile application
- **Alternative Solution**: Pivoted to web-based interface where ESP32 connects to personal hotspot and sends data to phone via web dashboard, providing cross-platform accessibility and alos enabling **Remote Patient Monitoring**

### Initial Algorithm Limitations
- **Problem**: Started with simple mathematical methods (averages, basic calculations) for mood detection algorithm, which failed to provide adequate accuracy
- **Solution**: Developed advanced algorithm using Z-score concept for better statistical analysis and improved mood classification accuracy

## Future Enhancements

- **Mobile Application**: Native iOS/Android app development
- **Dataset Analysis**: Analyze more datasets to find perfect thresholds for improved mood detection accuracy
- **Algorithm Exploration**: Experiment with different algorithms beyond current Z-score approach for better performance
- **Dynamic Feedback System**: Ask users to verify detected moods at specific intervals and adjust thresholds dynamically based on feedback

## Acknowledgments

- Hackathon organizing committee
- Sensor manufacturers for technical documentation
- Open-source community for reference implementations

## Contact Information
- Satyaram Mangena --- Satyaram.Mangena@iiitb.ac.in (Lead)
- Nihit Reddy --- Nihit.Reddy@iiitb.ac.in (Developer)
- Lithin Sai Kumar--- Lithin.SaiKumar@iiitb.ac.in (Tester)

---

