
# pip install ultralytics opencv-python pyserial

import os
import time
import cv2
import requests
import serial
import datetime
from ultralytics import YOLO

MAIN_BOARD_COM = 'COM3'
### Must SYNC with 'char' given in Main Arduino file.
ALERT_BAURD_RATE = 115200
HUMAN_DETECTION_CHAR = b'H'   

# Network and Stream Settings
CCTV_RTSP_URL = "rtsp://username:password@192.168.1.20:554/stream1"
ESP32_IP = "http://192.168.1.50"


VISION_MODEL = "yolov8n.pt"
# ----------------------------------------------------------------------

current_hour = datetime.datetime.now().hour

# If the current time is during the day (between 6:00 AM and 6:00 PM)
# --------------------------------------------------------------------
#if 6 <= current_hour < 18:
#    print("Power returned during daytime. Automatically going back to sleep to save electricity...")
#    # Force the computer back into low-power sleep mode immediately
#    os.system("rundll32.exe powrprof.dll,SetSuspendState 0,1,0")
#    exit()


# 1. Initialize the USB connection to your Arduino Nano
# --------------------------------------------------------------------
# Replace 'COM3' with the actual COM port your Nano uses (check Arduino IDE -> Tools -> Port)
try:
    arduino = serial.Serial(port=MAIN_BOARD_COM, baudrate=ALERT_BAURD_RATE, timeout=1)
    time.sleep(2)  # Give Arduino time to reset after plugging in
    print("🔌 Successfully connected to Arduino Nano alarm.")
except Exception as e:
    print(f"❌ Could not open Serial Port: {e}")
    arduino = None

# 2. Load the lightweight AI model (Uses your RTX GPU)
# --------------------------------------------------------------------
model = YOLO(VISION_MODEL)

# 3. Start Processing
cap = cv2.VideoCapture(CCTV_RTSP_URL)
last_alert_time = 0
ALERT_COOLDOWN = 10  # Seconds to wait before sounding the buzzer again

while cap.isOpened():
    ret, frame = cap.read()
    if not ret:
        break

    results = model(frame, verbose=False)

    human_detected = False
    for r in results.boxes:
        # Class 0 is human. Ensure confidence is over 50%
        if int(r.cls) == 0 and r.conf > 0.5:
            human_detected = True
            break

    if human_detected:
        current_time = time.time()
        # Only trigger alarm if cooldown period has passed (prevents endless buzzing)
        if current_time - last_alert_time > ALERT_COOLDOWN:
            print("🚨 HUMAN DETECTED! Sending hardware trigger.")
            last_alert_time = current_time

            # Trigger 1: Send local USB signal to your bedroom Arduino Nano
            if arduino and arduino.is_open:
                arduino.write(HUMAN_DETECTION_CHAR)

            # Trigger 2: Send over Wi-Fi to your rooftop ESP32 web server (optional)
            try:
                requests.get(f"{ESP32_IP}/trigger_alarm", timeout=1)
            except requests.exceptions.RequestException:
                pass

    if cv2.waitKey(1) & 0xFF == ord("q"):
        break

if arduino:
    arduino.close()
cap.release()
cv2.destroyAllWindows()
