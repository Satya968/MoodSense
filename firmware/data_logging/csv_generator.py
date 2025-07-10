import serial
import csv
import os
from datetime import datetime

# === USER CONFIGURATION ===
PORT = 'COM10'            # Change to your actual COM port (e.g., COM4, /dev/ttyUSB0)
BAUD_RATE = 115200       # Match your Serial.begin() speed
OUTPUT_FILE = os.path.expanduser("~/Desktop/final-test-nihit.csv")


# === INITIALIZE SERIAL PORT ===
try:
    ser = serial.Serial(PORT, BAUD_RATE, timeout=2)
    print(f"[INFO] Connected to {PORT} at {BAUD_RATE} baud.")
except:
    print(f"[ERROR] Could not open port {PORT}")
    exit()

# === PREPARE OUTPUT FILE ===
with open(OUTPUT_FILE, 'w', newline='') as csvfile:
    writer = csv.writer(csvfile)
    writer.writerow(["Timestamp", "HeartRate", "Temperature", "GSR"])  # CSV header

    print("[INFO] Logging started. Press Ctrl+C to stop.\n")
    try:
        while True:
            line = ser.readline().decode('utf-8', errors='ignore').strip()

            if line and line.count(',') == 3:
                fields = line.split(',')
                if all(fields):
                    writer.writerow(fields)
                    print(fields)  # Also show in console
    except KeyboardInterrupt:
        print("\n[INFO] Logging stopped by user.")
    finally:
        ser.close()
        print(f"[INFO] Port {PORT} closed.")
