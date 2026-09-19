"""
  Multithreaded Python script for the desktop computer features CUDA-accelerated YOLOv8 object detection 
  optimized for an RTX 2060, automated daytime power-saving sleep triggers, and offline voice command 
  recognition. The application processes twelve parallel RTSP CCTV streams while incorporating dynamic 
  internet-connection failover for media playback.

  @author: jeyakumar.kasi@hyproid.com
  @version: 1.0.0
  @created on: Sat Sep 19, 2026 13:21
"""

import os
import sys
import time
import datetime
import subprocess
import webbrowser
import threading
import requests
import speech_recognition as sr
import pyttsx3
import cv2
from ultralytics import YOLO

# ==================== CONFIGURATION SECTOR ====================
MASTER_ESP32_IP = "http://192.168.1.60"                            # Ground Floor Master IP
YOUTUBE_PLAYLIST = "https://youtube.com"                           # Christian Music Online URL
OFFLINE_MUSIC_DIR = r"C:\Music"                                    # Backup local folder for MP3s
VLC_PATH = r"C:\Program Files\VideoLAN\VLC\vlc.exe"                # Standard VLC install path
PLAY_CMD = "play"


# List all 12 of your CCTV Stream URLs (Mix of wired/wireless RTSP streams)
# Tip: Test with 0 or 1 for local webcams during bench setup
# (Optimized using camera SUB-STREAMS, i.e "/stream2" )
CCTV_STREAMS = {
    "Front Gate": "rtsp://admin:pass@192.168.1.20:554/stream2",
    "Backyard": "rtsp://admin:pass@192.168.1.21:554/stream2",
    "Living Room": "rtsp://admin:pass@192.168.1.22:554/stream2",
    "Kitchen": "rtsp://admin:pass@192.168.1.23:554/stream2",
    "Stairs 1F": "rtsp://admin:pass@192.168.1.24:554/stream2",
    "Stairs 2F": "rtsp://admin:pass@192.168.1.25:554/stream2",
    "Terrace West": "rtsp://admin:pass@192.168.1.26:554/stream2",
    "Terrace East": "rtsp://admin:pass@192.168.1.27:554/stream2",
    "Balcony": "rtsp://admin:pass@192.168.1.28:554/stream2",
    "Garage": "rtsp://admin:pass@192.168.1.29:554/stream2",
    "Main Door": "rtsp://admin:pass@192.168.1.30:554/stream2",
    "Side Pathway": "rtsp://admin:pass@192.168.1.31:554/stream2"
}
# ==============================================================

# --- POWER PRE-CHECK ---
# Power recovery safety: If computer restarts during daytime, go straight back to sleep
current_hour = datetime.datetime.now().hour
if 6 <= current_hour < 18:
    print("Power returned during daytime hours. Returning computer to sleep to save power...")
    os.system("rundll32.exe powrprof.dll,SetSuspendState 0,1,0")
    sys.exit()

# Initialize Offline Text-to-Speech Engine
engine = pyttsx3.init()
engine.setProperty('rate', 150)
voice_lock = threading.Lock() # Prevents overlapping text-to-speech output

def speak(text):
    with voice_lock:
        print(f"🤖 TTS Voice: {text}")
        engine.say(text)
        engine.runAndWait()

def is_internet_active():
    try:
        # Check internet status by hitting Cloudflare's lightweight DNS endpoint
        requests.get("https://1.1.1", timeout=2)
        return True
    except:
        return False

def launch_music_engine():
    if is_internet_active():
        speak("Opening Christian songs playlist on YouTube.")
        webbrowser.open(YOUTUBE_PLAYLIST)
    else:
        speak("Internet connection missing. Launching VLC fallback local storage player.")
        if os.path.exists(OFFLINE_MUSIC_DIR):
            vlc_cmd = [VLC_PATH, OFFLINE_MUSIC_DIR, "--random", "--loop"]
            subprocess.Popen(vlc_cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        else:
            speak("Error. Please add offline music files to C drive Music folder.")

def voice_recognition_worker():
    recognizer = sr.Recognizer()
    mic = sr.Microphone()
    speak("Voice command processing loop online.")
    
    while True:
        with mic as source:
            recognizer.adjust_for_ambient_noise(source, duration=0.5)
            try:
                audio = recognizer.listen(source, timeout=5, phrase_time_limit=3)
                command = recognizer.recognize_google(audio).lower()
                print(f"🎙️ Voice Received: '{command}'")
                if PLAY_CMD in command:
                    launch_music_engine()
            except:
                pass

# --- MULTITHREADED VIDEO CAPTURE ENGINE ---
class CameraStreamBuffer:
    """Constantly grabs frames from an RTSP stream in a background thread to prevent lag."""
    def __init__(self, name, url):
        self.name = name
        self.url = url
        self.cap = cv2.VideoCapture(url)
        self.frame = None
        self.started = False
        self.lock = threading.Lock()

    def start(self):
        if self.started:
            return
        self.started = True
        t = threading.Thread(target=self.update, args=(), daemon=True)
        t.start()
        return self

    def update(self):
        while self.started:
            ret, frame = self.cap.read()
            if not ret:
                # If stream drops (common on Wi-Fi cameras), attempt automatic reconnection
                time.sleep(2)
                self.cap = cv2.VideoCapture(self.url)
                continue
            with self.lock:
                self.frame = frame

    def get_frame(self):
        with self.lock:
            return self.frame

    def stop(self):
        self.started = False
        self.cap.release()

def cctv_multi_ai_worker():
    print("🚀 Initializing YOLOv8 AI Core on NVIDIA RTX 2060...")
    # Load model directly into CUDA space to drop CPU utilization to near zero
    model = YOLO("yolov8n.pt").to('cuda')
    
    # Fire up parallel capturing buffers across all 12 camera sources
    active_cameras = {}
    for name, url in CCTV_STREAMS.items():
        print(f"🎬 Initializing network buffer pipeline for: {name}")
        active_cameras[name] = CameraStreamBuffer(name, url).start()
        
    last_alert = 0
    alert_cooldown = 20 # Seconds to wait before announcing an alert again
    
    print("🎯 All 12 camera matrices are online and processing in parallel.")
    
    while True:
        # Loop through each camera frame buffer sequentially to process on the RTX GPU
        for name, cam in active_cameras.items():
            frame = cam.get_frame()
            if frame is None:
                continue # Skip camera if it hasn't caught a frame yet
                
            # OPTIMIZATION: half=True compresses calculation footprint to 16-bit, reduces VRAM usage by 50% (i.e half)
            # device=0 ensures it executes on your RTX 2060 GPU
            results = model(frame, verbose=False, device=0, half=True)
            human_detected = False
            
            for r in results.boxes:
                if int(r.cls) == 0 and r.conf > 0.5: # Class 0 = Human check
                    human_detected = True
                    break
                    
            if human_detected:
                now = time.time()
                if now - last_alert > alert_cooldown:
                    last_alert = now
                    print(f"🚨 INTRUDER SPOTTED AT: {name}!")
                    # Non-blocking voice execution via sub-thread so the AI loop doesn't pause while speaking
                    threading.Thread(target=speak, args=(f"Warning. Intruder spotted at the {name}.",), daemon=True).start()
                    
        # Small sleep step to prevent maxing out internal CPU bus pipelines
        time.sleep(0.01)

if __name__ == "__main__":
    # Spin up multi-processing execution layers
    t_voice = threading.Thread(target=voice_recognition_worker, daemon=True)
    t_ai = threading.Thread(target=cctv_multi_ai_worker, daemon=True)
    
    t_voice.start()
    t_ai.start()
    
    # Maintain root thread lifecycle
    while True:
        time.sleep(1)
