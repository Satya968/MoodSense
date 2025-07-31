# **MoodSense**

**A Real-Time Physiological Monitoring System for Relapse Prevention and Recovery Support in deaddiction phase**
##**Overview-Revolutionizing Addiction Recovery Through Continuous Mood Surveillance**
MoodSense addresses one of the most critical challenges in addiction recovery: **objective tracking and reporting of mood patterns over time**. Mood disorders, including depression and bipolar disorders, are the most common psychiatric comorbidities among patients with substance use disorders, making continuous mood monitoring essential for successful recovery outcomes.

Our system leverages multi-sensor physiological data to provide objective mood history and patterns, enabling healthcare providers, counselors, and individuals in recovery to understand emotional trends and make informed decisions about treatment and support strategies.

---

## **The Science Behind Addiction & Mood Monitoring**

### Why Mood Monitoring Matters in Recovery

When done properly with pen and paper or an electronic mood tracker, mood journaling can be the difference maker in a person maintaining sobriety in recovery, and may even help highlight underlying causes for substance use in the first place. However, traditional mood tracking relies on subjective self-reporting, which can be incomplete or forgotten during busy periods.

Biosensors embedded in wearable devices can continuously monitor the physiological signatures of emotional states, like heart rate and skin conductance, providing objective historical data that shows what moods actually occurred over the past hours and days, complementing traditional recovery approaches with factual mood history.

### Clinical Applications

- **Mood History Reporting**: Provide objective records of mood states over recent hours/days
- **Pattern Recognition**: Identify recurring mood cycles and emotional trends
- **Treatment Assessment**: Show how mood patterns correlate with different treatments
- **Recovery Progress Documentation**: Objective measurement of emotional stability over time
- **Therapy Support**: Give counselors factual mood data to guide session discussions

---

## **How MoodSense Works**

### Multi-Sensor Physiological Monitoring

Our system continuously monitors three key physiological parameters that correlate with emotional states:

1. **Galvanic Skin Response (GSR)**: Measures skin conductance variations indicating stress and emotional arousal
2. **Heart Rate Variability**: Captures autonomic nervous system responses to mood changes
3. **Body Temperature**: Monitors subtle temperature fluctuations associated with emotional states

### Personalized Baseline Establishment

Unlike generic mood tracking apps, MoodSense establishes individual physiological baselines through guided meditation sessions, ensuring personalized and accurate mood detection for each user's unique physiological patterns.

### Advanced Signal Processing

The system employs Z-score statistical analysis and threshold-based algorithms to classify mood states into four categories based on physiological data collected over the past periods:
- **Happy** 😊 - Positive emotional state with balanced physiological markers
- **Calm** 😌 - Relaxed state with stable baseline readings
- **Sad** 😢 - Depressed mood with characteristic physiological signatures
- **Stressed** 😰 - Elevated arousal indicating heightened emotional activity

---

## **Technical Implementation**

### Hardware Components

| Component | Function | Specification |
|-----------|----------|---------------|
| **ESP32 Microcontroller** | Main processing unit | Wi-Fi enabled, real-time data processing |
| **MAX30102 Sensor** | Heart rate monitoring | ±3 BPM accuracy, validated against commercial devices |
| **DS18B20 Sensor** | Body temperature sensing | ±0.5°C accuracy, drift-characterized |
| **GSR Sensor Module** | Skin conductance measurement | 0-4095 range, pressure-compensated algorithm |

### System Specifications

- **Sampling Rate**: 30-second intervals for comprehensive monitoring
- **Detection Frequency**: Mood classification every 45 minutes
- **Power Consumption**: 1.28A at 3.3V (12+ hours continuous operation)
- **Connectivity**: Bluetooth enabled for mobile app integration
- **Data Storage**: Local storage on mobile device with daily report generation
- **Accuracy**: 50% accuracy against ground truth validation

---

## **Sensor Characterization & Validation**

### Comprehensive Sensor Testing

#### MAX30102 Heart Rate Sensor Validation
- **Validation Method**: Compared against commercial pulse oximeter
- **Test Duration**: 1-hour continuous monitoring
- **Sampling Rate**: 30-second intervals
- **Result**: ±3 BPM accuracy achieved through custom algorithm development

#### DS18B20 Temperature Sensor Validation  
- **Validation Method**: Compared with IR temperature gun measurements
- **Test Conditions**: Multiple temperature ranges and environmental conditions
- **Result**: ±0.5°C accuracy maintained across operational range

#### GSR Sensor Characterization
- **Challenge Addressed**: Pressure-dependent readings causing inconsistent measurements
- **Solution Implemented**: Differential analysis using personalized baseline values
- **Validation**: 15-hour continuous monitoring with drift characterization

### Long-term Stability Testing

#### 15-Hour Sensor Drift Analysis
- **Focus**: Long-term sensor stability ,sensor drift and calibration maintenance
- **Outcome**: Comprehensive drift characterization for all sensors
  
#### 12-Hour Continuous Operation Test
- **Purpose**: Validate system stability
- **Parameters Monitored**: All sensors simultaneously
- **Results**: Stable operation with minimal drift
- **Data Collection**: 2 readings per minute (2,880 data points total) and mood detected once in every 45 minutes 


---

## **Installation & Setup**

### Hardware Assembly

1. **Component Connection**: Wire sensors to ESP32 according to provided schematic
2. **Power Supply**: Ensure stable 3.3V/5V power supply with proper grounding
3. **Sensor Calibration**: Verify individual sensor functionality using test libraries
4. **System Integration**: Complete assembly and connection verification

### Software Installation

#### Main Monitoring System
```bash
# Navigate to firmware directory
cd firmware/src/

# Required Arduino Libraries:
- BluetoothSerial
- ArduinoJson
- Wire
- MAX30105
- OneWire
- DallasTemperature
- math

# Upload moodSense.ino to ESP32 (with Bluetooth enabled)
```

#### Flutter iOS Application
```bash
# Navigate to mobile app directory firmware/app/ and follow readme.md

# Flutter dependencies:
flutter pub get

# Build and install on iOS device
flutter build ios
flutter install
```

### Mobile App Configuration

1. **Bluetooth Pairing**: Pair ESP32 device with iOS device
2. **App Installation**: Install MoodSense Flutter app on iOS device
3. **Device Connection**: Connect to ESP32 via Bluetooth within the app
4. **Data Synchronization**: Real-time sensor data streaming to mobile device

---

## **Data Output & Daily Report Generation**

### iOS App Features & Daily Reports
- **Real-Time Monitoring**: Live sensor data display with Bluetooth connectivity
- **Daily Report Button**: One-click generation of comprehensive daily analytics
- **Visual Analytics**: Four detailed graphs showing sensor trends and mood fluctuations
- **Data Tables**: Complete tabulated data with timestamps for clinical use

### Comprehensive Daily Report Contents

#### **Four Visual Graphs**
1. **GSR (Galvanic Skin Response) Trends**: 24-hour skin conductance patterns
2. **Heart Rate Variability**: Heart rate fluctuations throughout the day
3. **Body Temperature Changes**: Temperature variations and patterns
4. **Mood Fluctuation Timeline**: Mood state changes with timestamps

#### **Detailed Data Tables**
- **Sensor Readings**: Complete timestamp data for all three sensors
- **Mood Classifications**: Timestamped mood states with confidence levels
- **Statistical Summary**: Daily averages, peaks, and pattern analysis
- **Session Metadata**: Calibration status and data quality metrics

### Data Export & Clinical Integration
- **PDF Report Generation**: Professional reports for clinical appointments
- **Data Sharing**: Easy sharing via email or messaging apps
- **Local Storage**: Secure on-device data storage with privacy protection
- **Clinical Compatibility**: Reports formatted for healthcare provider review

---

## **Usage for Addiction Recovery**

### Initial Setup & Calibration

1. **Baseline Establishment**
   - Perform 1-2 minute guided meditation session
   - System records stable physiological readings
   - Personalized thresholds calculated automatically
   - Calibration status displayed on app UI

2. **Continuous Mood Logging**
   - Wear sensors during daily activities
   - ESP32 continuously samples sensors every 30 seconds
   - Data streams to iOS app via Bluetooth
   - Mood classification performed every 45 minutes
   - All data stored locally on mobile device
     
### Clinical Integration

#### For Healthcare Providers
- **Daily Report Review**: Receive comprehensive daily reports from patients
- **Objective Assessment**: Supplement subjective reports with physiological mood history
- **Treatment Evaluation**: Assess how different treatments affected mood over time
- **Appointment Preparation**: Review detailed sensor data and mood patterns before sessions

#### For Recovery Counselors
- **Session Enhancement**: Discuss actual mood patterns using detailed daily reports
- **Progress Tracking**: Show concrete evidence of emotional stability improvements through visual graphs
- **Trigger Analysis**: Correlate past mood changes with life events using timestamped data
- **Treatment Planning**: Use comprehensive daily statistics to adjust counseling approaches

#### For Individuals in Recovery
- **Daily Self-Review**: Generate and review daily mood and physiological reports
- **Memory Aid**: Remember emotional experiences with detailed timestamps and data
- **Recovery Documentation**: Track emotional progress with visual graphs and statistics
- **Awareness Building**: Learn about personal patterns through comprehensive daily analytics

---

##  **Future Development for Recovery Applications**

### Enhanced Clinical Features
- **Extended Historical Analysis**: Longer-term mood pattern analysis (months/years)
- **Integration with Electronic Health Records**: Include mood history in clinical records
- **Multi-Patient Dashboard**: Clinical review of patient mood histories
- **Comparative Analysis**: Compare mood patterns across different treatment phases

### Advanced Analytics
- **Machine Learning Integration**: Improved historical mood pattern recognition
- **Longitudinal Analysis**: Long-term emotional stability assessment
- **Pattern Correlation**: Link mood patterns with treatment effectiveness
- **Recovery Timeline Mapping**: Visual representation of emotional recovery journey

### Mobile Application Enhancement
- **Android Version**: Extend Flutter app to Android platform
- **Advanced Report Features**: Weekly and monthly report generation
- **Data Synchronization**: Cloud backup and multi-device sync
- **Enhanced Visualizations**: Interactive charts and trend analysis

### Research & Validation
- **Longitudinal Studies**: Support for long-term mood pattern research
- **Dataset Expansion**: Larger validation studies for improved historical accuracy
- **Algorithm Optimization**: Better mood classification from physiological data
- **Pattern Recognition Enhancement**: Improved identification of mood cycles and trends

---

## **Technical Challenges & Solutions**

### Sensor Reliability Issues

#### GSR Sensor Pressure Dependency
- **Problem**: Highly pressure-sensitive readings causing inconsistent measurements
- **Solution**: Implemented differential analysis using baseline-established values
- **Result**: Stable readings independent of application pressure

#### Heart Rate Sensor Accuracy
- **Problem**: SparkFun example code yielded consistently high readings (>100 BPM)
- **Solution**: Developed custom algorithm based on heart rate plotter methodology
- **Result**: Achieved realistic and accurate heart rate measurements

### System Integration Challenges

#### Mobile Application Development
- **Previous Approach**: Web-based interface with Wi-Fi connectivity
- **New Solution**: Native Flutter iOS app with Bluetooth connectivity for better portability
- **Result**: Seamless mobile integration with offline capability and professional report generation

#### Data Visualization & Reporting
- **Challenge**: Providing comprehensive daily analytics for clinical use
- **Solution**: Implemented one-click daily report generation with four detailed graphs and data tables
- **Result**: Professional-grade reports suitable for healthcare providers and personal review

#### Algorithm Development
- **Problem**: Simple mathematical approaches provided inadequate accuracy
- **Solution**: Implemented advanced Z-score statistical analysis
- **Result**: Improved mood classification accuracy and reliability

---

## **Clinical Impact & Applications**

### Evidence-Based Recovery Support

The integration of objective mood history in addiction recovery addresses critical gaps in traditional treatment approaches. Having factual records of mood patterns helps both patients and providers understand emotional trends and evaluate treatment effectiveness over time.

### Historical Mood Documentation Benefits

Continuous mood logging provides unprecedented insight into patient emotional patterns, offering objective data that complements self-reported experiences. This historical perspective enables better understanding of what treatments work and how emotional stability develops during recovery.

### Research Applications

The comprehensive sensor characterization and validated accuracy make MoodSense suitable for longitudinal clinical research applications, contributing to understanding of mood patterns during different phases of addiction recovery.

---

## 👥 **Development Team**

**IIIT Bangalore - ELCIA Sense2Scale 2025 Hackathon**

- **Satyaram Mangena** - [Satyaram.Mangena@iiitb.ac.in](mailto:Satyaram.Mangena@iiitb.ac.in) - Lead Developer & Project Coordinator
- **Nihit Reddy** - [Nihit.Reddy@iiitb.ac.in](mailto:Nihit.Reddy@iiitb.ac.in) - Algorithm Development & System Integration  
- **Lithin Sai Kumar** - [Lithin.SaiKumar@iiitb.ac.in](mailto:Lithin.SaiKumar@iiitb.ac.in) - Testing & Validation

---

## 📝 **Acknowledgments**

- **ELCIA Sense2Scale 2025 Hackathon** - Platform for innovation and development
- **Addiction Recovery Community** - Inspiration and feedback for clinical applications
- **Sensor Manufacturers** - Technical documentation and support  
- **Open Source Community** - Reference implementations and collaborative development

---

**Disclaimer**: MoodSense is a research tool designed to support addiction recovery efforts. It should not be used as the sole basis for clinical decisions and must be integrated with comprehensive professional treatment programs.

---

*"Recovery is not a destination, but a journey. MoodSense provides the historical perspective to understand the emotional landscape of that journey with objective insight and professional support."*
