# MoodSense
Continuous Mood &amp; Mental Health Logger for Remote Patient Monitoring
# try
# Physiological Mood Detection System

## Overview

This project implements a real-time mood detection system using physiological sensors including GSR (Galvanic Skin Response), heart rate monitoring via MAX30102, and temperature sensing with DS18B20. The system establishes personalized baselines through meditation sessions and uses advanced signal processing algorithms to detect emotional states.

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
- **Accuracy**: Achieved X% accuracy against ground truth

## Installation and Setup

### Hardware Setup
1. Connect sensors to ESP32 according to wiring diagram
2. Ensure proper power supply and grounding
3. Verify sensor connectivity and calibration

### Software Installation
```bash
# Clone repository
git clone [repository-url]

# Install dependencies
pip install -r requirements.txt

# Flash ESP32 firmware
# [Include specific flashing instructions]
```

### Network Configuration
1. Configure Wi-Fi credentials in ESP32 code
2. Deploy web interface to local network
3. Access dashboard via provided IP address

## Usage

### Initial Calibration
1. Power on the system
2. Follow meditation guide for baseline establishment
3. Confirm successful calibration

### Continuous Monitoring
1. Wear sensors as instructed
2. Access web dashboard for real-time monitoring
3. Review mood detection results every 45 minutes

## Data Output

### Sensor Data
- **Format**: CSV with timestamps
- **Frequency**: 2 readings per minute
- **Parameters**: GSR, heart rate, temperature, current consumption

### Mood Detection
- **Frequency**: Every 45 minutes
- **Output**: Classified mood state with confidence metrics
- **Logging**: Comprehensive mood history with timestamps

## Technical Specifications

### Sensor Specifications
- **GSR**: [Include range and accuracy]
- **MAX30102**: Heart rate ±2 BPM, SpO2 ±2%
- **DS18B20**: ±0.5°C accuracy

### Performance Metrics
- **Detection Latency**: < 1 second
- **Power Consumption**: [Include measured values]
- **Continuous Operation**: 12+ hours validated

## Problems Faced and Solutions We Came Up With

### Current Limitations
- **GSR Sensitivity**: Readings affected by contact pressure
- **Individual Variations**: Person-to-person baseline differences
- **Environmental Factors**: Temperature and humidity sensitivity

### Mitigation Strategies
- **Pressure Calibration**: Standardized sensor placement protocol
- **Personalized Baselines**: Individual calibration requirement
- **Environmental Monitoring**: Ambient condition logging

## Future Enhancements

### Planned Improvements
- **Mobile Application**: Native iOS/Android app development
- **Extended Sensor Suite**: Additional physiological parameters
- **Machine Learning Integration**: Advanced pattern recognition
- **Cloud Connectivity**: Remote monitoring capabilities

### Research Directions
- **Long-term Studies**: Extended validation periods
- **Population Studies**: Demographic-specific calibration
- **Clinical Validation**: Medical-grade accuracy assessment

## Challenges and Solutions

### Technical Challenges Encountered

#### Heart Rate Sensor Integration
- **Challenge**: Initial MAX30102 implementation using SparkFun examples showed impractical results
- **Solution**: Developed custom algorithm based on heart rate plotter examples
- **Timeline**: 2 days of troubleshooting and optimization

#### Mobile App Development
- **Challenge**: iOS development complexity with team's Android experience
- **Solution**: Pivoted to web-based interface accessible via browser
- **Advantage**: Cross-platform compatibility and easier deployment

#### GSR Signal Processing
- **Challenge**: Pressure-dependent readings and inter-individual variations
- **Solution**: Implemented differential analysis and personalized baseline calibration
- **Result**: Improved accuracy and reduced false positives

### Ongoing Challenges
- **Sensor Fusion**: Optimizing multi-sensor data integration
- **Real-time Processing**: Balancing accuracy with response time
- **User Experience**: Simplifying calibration procedures

## Contributing

### Development Guidelines
- Follow established coding standards
- Document all sensor calibration procedures
- Validate changes against reference devices
- Maintain comprehensive test coverage

### Testing Requirements
- Hardware-in-the-loop validation
- Cross-platform compatibility testing
- Performance benchmarking
- User acceptance testing

## License

[Include appropriate license information]

## Acknowledgments

- Hackathon organizing committee
- Sensor manufacturers for technical documentation
- Open-source community for reference implementations

## Contact Information

[Include team contact details]

---

**Note**: This system is designed for research and development purposes. For medical or clinical applications, additional validation and regulatory approval may be required.
