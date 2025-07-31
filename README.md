# MoodSense
Continuous Mood Monitoring for Addiction Recovery (De-addiction Support)

## Overview

**MoodSense** is an open-source, real-time mood monitoring system tailored for supporting individuals undergoing addiction recovery or de-addiction programs. Leveraging low-cost physiological sensors and personalized calibration, MoodSense enables clinicians, caregivers, and patients to track emotional well-being remotely and objectively—crucial for early intervention during recovery phases.

Originally designed as a general mental health logger, MoodSense now specifically addresses the need for **continuous mood and stress monitoring in addiction therapy**, helping caregivers detect patterns, triggers, and relapse risks through actionable, data-driven insights.

## New Relevance: Why Mood Monitoring in De-addiction?

- **Clinically significant:** Emotional state is a key predictor for relapse and withdrawal complexity.
- **Personalized support:** Enables therapists to tailor interventions based on objective, historical trends rather than only self-reported data.
- **Early warning:** Alerts when patients experience increased stress, sadness, or agitation, potentially preceding cravings or relapse.
- **Remote care:** Empowers remote patient monitoring (“virtual ward rounds”) for outpatient or home settings.

## Features

- **Multi-sensor Integration:** Combines GSR (Galvanic Skin Response), heart rate (MAX30102), and temperature (DS18B20) for holistic mood detection.
- **Personalized Baseline Calibration:** Individual thresholds set through guided meditation, countering inter-user physiological variability.
- **Continuous Tracking:** Mood state updated every 45 minutes—invaluable for detecting rapid mood changes typical in withdrawal phases.
- **Web-Based Dashboard:** Secure, cross-platform interface for real-time supervision by clinics or caregivers.
- **Comprehensive Logging:** Captures granular physiological data and mood states with timestamped history.
- **Accuracy Validation:** Empirically benchmarked against clinical-grade devices (details below).

## System Architecture

| Layer            | Components & Functions                                                                                  |
|------------------|--------------------------------------------------------------------------------------------------------|
| **Sensor Layer** | - **GSR:** Skin conductance (stress/arousal proxy)<br/>- **MAX30102:** Heart rate, SpO₂<br/>- **DS18B20:** Body temperature |
| **Processing**   | - Baseline calibration<br/>- Z-score computation<br/>- Multi-variate mood classification               |
| **Interface**    | - Web dashboard<br/>- CSV export<br/>- Wi-Fi/IoT connectivity                                          |

## Hardware Components

- ESP32 microcontroller (core processing & Wi-Fi)
- MAX30102 heart rate and SpO₂ sensor
- DS18B20 digital temperature sensor
- GSR module (skin conductance)
- Support electronics per schematics

## Mood Detection Algorithm

1. **Baseline Establishment:** 1–2 min guided meditation to determine individualized resting values.
2. **Data Collection:** Readings every 30 seconds, filtered for noise and artifacts.
3. **Feature Extraction:** Statistical (mean, stdev, Z-score), differential analysis.
4. **Mood Classification:** Combines sensor readings via threshold decision trees (Z-score based).
5. **Output:** Classifies mood (Happy, Calm, Sad, Stressed) every 45 mins with confidence metrics.

## Clinical/Validation Data & Sensor Characterization

### Sensor Accuracy

- **GSR:** 0–4095 raw; corrected for user/sensor pressure via baseline differential.
- **MAX30102:** Heart rate ±3 BPM, validated vs commercial pulse oximeter (1hr, every 30s).
- **DS18B20:** ±0.5°C accuracy, benchmarked with IR temp gun.

### Performance Metrics

| Metric                  | Value/Result                                |
|-------------------------|---------------------------------------------|
| **Sampling Rate**       | 2 readings/minute                           |
| **Detection Interval**  | 45 minutes (mood state)                     |
| **Battery/Power**       | 1.28A @ 3.3V (tested up to 12hrs continuous)|
| **Validation**          | ~50% mood detection accuracy against ground truth (manual verification every 45 mins, 12hr session) |
| **Long-term Stability** | 15hr drift analysis, low drift observed following baseline correction               |

## Development Highlights & Challenges

- **GSR Pressure Sensitivity:** Solved via differential algorithm using calibration.
- **Heart Rate Accuracy:** Custom processing algorithm improved over library defaults.
- **Mobile Constraints:** Cross-platform web dashboard (ESP32 connects via local Wi-Fi/hotspot).
- **Advanced Algorithm:** Z-score-based feature fusion significantly improved classification robustness.

## Installation & Setup

See `/firmware/` and detailed instructions in the repo for:
- **Hardware setup:** Schematics, sensor checks.
- **Software flashing:** Required Arduino libraries and firmware files.
- **Data logging:** Utilities for generating/exporting CSV datasets.
- **Network config:** Wi-Fi setup for remote dashboard access.

## Usage Workflow

1. Wear sensors and power system.
2. Complete baseline (“meditation”) calibration.
3. Access live dashboard via network IP.
4. Monitor mood dynamics, view/export logs, and adjust baselines as user physiology adapts.

## Future Development (De-addiction Focus)

- **Therapist Alerts:** Automatic flagging of high-risk mood patterns.
- **Mobile App:** Native cross-platform app for user convenience.
- **Dataset Expansion:** Enhance thresholds using broader population datasets.
- **Algorithm Improvement:** Explore ML models for robust classification.
- **Dynamic Feedback:** User mood self-verification for adaptive calibration.

## Contact

- **Satyaram Mangena (Lead):** Satyaram.Mangena@iiitb.ac.in
- **Nihit Reddy (Developer):** Nihit.Reddy@iiitb.ac.in
- **Lithin Sai Kumar (Testing):** Lithin.SaiKumar@iiitb.ac.in

*This project proudly pivoted to support the nuanced needs of remote addiction recovery monitoring after feedback at ELCIA Sense2Scale Hackathon 2025.*
