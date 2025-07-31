# Firmware

This folder contains the Arduino firmware for the mood detection system.

## Structure

### `src/`
- **[`moodSense.ino`](src/moodSense.ino)** - Main mood detection firmware
  - **Calibration**: First 2 minutes after startup
  - **Detection**: Mood analysis every 45 minutes
  - **Sampling**: 2 sensor readings collected every minute

### `data_logging/`
- **[`sensor_logger.ino`](data_logging/sensor_logger.ino)** - Data collection firmware for training/testing
  - Collects 2 sensor readings every minute
  - Outputs data via serial for logging
- **[`csv_generator.py`](data_logging/csv_generator.py)** - Python script to generate CSV from sensor data
  - Runs parallel to Arduino
  - Reads serial data and creates CSV files
  - **Important**: Close Arduino Serial Monitor before running Python script

### `app/`
- **[`Info.plist`](app/Info.plist)**
- **[`pubspec.yaml`](app/pubspec.yaml)**
- **[`main.dart`](app/main.dart)**
## Usage

**For mood detection:**
```bash
# Upload src/moodSense.ino to Arduino
```

**For data logging:**
```bash
# 1. Upload data_logging/sensor_logger.ino to Arduino
# 2. Close Serial Monitor
# 3. Run Python script
python csv_generator.py
```

**For app:**
```bash
#Create a new flutter folder
flutter create app_name

#replace Info.plist,pubspec.yaml and main.dart

#run
flutter pub get
flutter run
```

## Notes
- Ensure Serial Monitor is closed when using [`csv_generator.py`](data_logging/csv_generator.py)
- Python script requires serial access to Arduino
- Change the port number as per your connections in [`csv_generator.py`](data_logging/csv_generator.py)
