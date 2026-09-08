
# pip install ultralytics opencv-python requests pyserial SpeechRecognition pyttsx3 pyaudio pillow



import tkinter as tk
from tkinter import ttk
import cv2
import threading
import time
import webbrowser
import requests
import serial
import speech_recognition as sr
import pyttsx3
from PIL import Image, ImageTk
from ultralytics import YOLO

# ==================== CONFIGURATION SECTOR ====================
ARDUINO_COM_PORT = "COM3"       # Bedroom Arduino Nano USB Port
ESP32_IP = "http://192.168.1.50" # Rooftop ESP32 Local Static IP
CCTV_RTSP_URL = 0               # Use 0 for local webcam test, or replace with "rtsp://..."
CHRISTIAN_PLAYLIST = "https://youtube.com" # Replace with your playlist URL
# ==============================================================

class SmartHomeApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Unified Home & Security Hub")
        self.root.geometry("1050x650")
        self.root.configure(bg="#1e1e24")

        # Initialize Hardware & Engines
        self.init_hardware()
        self.init_voice_engine()
        self.model = YOLO("yolov8n.pt")

        # UI Setup
        self.create_widgets()

        # Threads Management
        self.running = True
        self.video_thread = threading.Thread(target=self.video_loop, daemon=True)
        self.video_thread.start()
        
        self.voice_thread = threading.Thread(target=self.voice_loop, daemon=True)
        self.voice_thread.start()
        
        self.data_thread = threading.Thread(target=self.fetch_esp32_data, daemon=True)
        self.data_thread.start()

        # Handle window close safely
        self.root.protocol("WM_DELETE_WINDOW", self.on_close)

    def init_hardware(self):
        try:
            self.arduino = serial.Serial(port=ARDUINO_COM_PORT, baudrate=115200, timeout=1)
            time.sleep(1)
        except Exception:
            self.arduino = None

    def init_voice_engine(self):
        self.tts_engine = pyttsx3.init()
        self.tts_engine.setProperty('rate', 150)
        self.speak_queue = []
        # Background helper thread for non-blocking Text-To-Speech
        threading.Thread(target=self.tts_worker, daemon=True).start()

    def speak(self, text):
        self.speak_queue.append(text)

    def tts_worker(self):
        while self.running:
            if self.speak_queue:
                text = self.speak_queue.pop(0)
                self.tts_engine.say(text)
                self.tts_engine.runAndWait()
            time.sleep(0.1)

    def create_widgets(self):
        # Header Style Title
        title = tk.Label(self.root, text="CENTRAL INTELLIGENCE DASHBOARD", font=("Helvetica", 18, "bold"), fg="#ffffff", bg="#1e1e24")
        title.pack(pady=15)

        # Main Layout Splitter
        main_frame = tk.Frame(self.root, bg="#1e1e24")
        main_frame.pack(fill=tk.BOTH, expand=True, px=20, py=10)

        # LEFT SIDE: Security Camera Feed
        self.left_frame = tk.LabelFrame(main_frame, text=" Live CCTV AI Analytics (RTX 2060) ", fg="#4ea8de", bg="#1e1e24", font=("Helvetica", 11, "bold"))
        self.left_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, px=10)
        
        self.video_label = tk.Label(self.left_frame, bg="#000000")
        self.video_label.pack(fill=tk.BOTH, expand=True, px=5, py=5)

        # RIGHT SIDE: Home Automation Control Module
        right_frame = tk.Frame(main_frame, bg="#1e1e24")
        right_frame.pack(side=tk.RIGHT, fill=tk.BOTH, px=10)

        # Block A: Water Management
        tank_frame = tk.LabelFrame(right_frame, text=" Rooftop Controls (ESP32) ", fg="#72efdd", bg="#1e1e24", font=("Helvetica", 11, "bold"))
        tank_frame.pack(fill=tk.X, py=5)

        self.lbl_tank = tk.Label(tank_frame, text="Water Level: Connecting...", font=("Helvetica", 12), fg="#ffffff", bg="#1e1e24", anchor="w")
        self.lbl_tank.pack(fill=tk.X, px=10, py=5)

        self.lbl_irrigation = tk.Label(tank_frame, text="Drip Valve: Connecting...", font=("Helvetica", 12), fg="#ffffff", bg="#1e1e24", anchor="w")
        self.lbl_irrigation.pack(fill=tk.X, px=10, py=5)

        btn_water = tk.Button(tank_frame, text="⚡ Force Trigger Drip Irrigation", font=("Helvetica", 10, "bold"), bg="#560bad", fg="#ffffff", command=self.trigger_irrigation)
        btn_water.pack(fill=tk.X, px=10, py=10)

        # Block B: Security Status Dashboard
        sec_frame = tk.LabelFrame(right_frame, text=" Security Alerts ", fg="#f72585", bg="#1e1e24", font=("Helvetica", 11, "bold"))
        sec_frame.pack(fill=tk.X, py=5)

        self.lbl_sec_status = tk.Label(sec_frame, text="SYSTEM SECURE", font=("Helvetica", 14, "bold"), fg="#4cc9f0", bg="#1e1e24", py=10)
        self.lbl_sec_status.pack(fill=tk.X)

        # Block C: Music Engine Controls
        music_frame = tk.LabelFrame(right_frame, text=" Music Hub ", fg="#ffb703", bg="#1e1e24", font=("Helvetica", 11, "bold"))
        music_frame.pack(fill=tk.X, py=5)

        lbl_voice_info = tk.Label(music_frame, text="Voice Trigger: Active\nSay 'PLAY' out loud anytime.", font=("Helvetica", 10, "italic"), fg="#aaaaaa", bg="#1e1e24")
        lbl_voice_info.pack(py=5)

        btn_play = tk.Button(music_frame, text="🎵 Manual Play Christian Playlist", font=("Helvetica", 10, "bold"), bg="#fb8500", fg="#ffffff", command=self.launch_playlist)
        btn_play.pack(fill=tk.X, px=10, py=10)

    def launch_playlist(self):
        self.speak("Starting your Christian songs playlist now. Enjoy.")
        webbrowser.open(CHRISTIAN_PLAYLIST)

    def trigger_irrigation(self):
        def network_call():
            try:
                res = requests.get(f"{ESP32_IP}/start_irrigation", timeout=2)
                self.speak(res.text)
            except Exception:
                self.speak("Communication failed. Check rooftop connection.")
        threading.Thread(target=network_call, daemon=True).start()

    def fetch_esp32_data(self):
        while self.running:
            try:
                res = requests.get(f"{ESP32_IP}/", timeout=2)
                lines = res.text.split('\n')
                self.lbl_tank.config(text=lines[0])
                self.lbl_irrigation.config(text=lines[1])
            except Exception:
                self.lbl_tank.config(text="Water Level: ESP32 Offline")
                self.lbl_irrigation.config(text="Drip Valve: Offline")
            time.sleep(5)

    def video_loop(self):
        cap = cv2.VideoCapture(CCTV_RTSP_URL)
        last_alert = 0
        
        while self.running and cap.isOpened():
            ret, frame = cap.read()
            if not ret:
                time.sleep(0.03)
                continue

            # Run YOLO AI Target Checking
            results = self.model(frame, verbose=False)
            human_spotted = False

            for r in results.boxes:
                if int(r.cls) == 0 and r.conf > 0.5:
                    human_spotted = True
                    # Draw visual target box over human profile
                    x1, y1, x2, y2 = map(int, r.xyxy[0])
                    cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 0, 255), 2)

            if human_spotted:
                self.lbl_sec_status.config(text="⚠️ INTRUDER ALERT! ⚠️", fg="#f72585")
                now = time.time()
                if now - last_alert > 10: # 10-second warning buffer limit
                    last_alert = now
                    if self.arduino and self.arduino.is_open:
                        self.arduino.write(b"H") # Strike Bedroom Buzzer via USB
                    try:
                        requests.get(f"{ESP32_IP}/trigger_alarm", timeout=1) # Pulse Rooftop Signal
                    except Exception: pass
            else:
                self.lbl_sec_status.config(text="🛡️ SYSTEM SECURE", fg="#4cc9f0")

            # Format raw frame array to layout into Tkinter canvas window
            frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            frame = cv2.resize(frame, (640, 480))
            img = Image.fromarray(frame)
            imgtk = ImageTk.PhotoImage(image=img)
            
            if self.running:
                self.video_label.imgtk = imgtk
                self.video_label.config(image=imgtk)
                
        cap.release()

    def voice_loop(self):
        recognizer = sr.Recognizer()
        mic = sr.Microphone()
        
        while self.running:
            with mic as source:
                recognizer.adjust_for_ambient_noise(source, duration=0.5)
                try:
                    audio = recognizer.listen(source, timeout=4, phrase_time_limit=2)
                    command = recognizer.recognize_google(audio).lower()
                    if "play" in command:
                        # Schedule browser link firing to avoid background network crash
                        self.root.after(0, self.launch_playlist)
                except Exception:
                    pass

    def on_close(self):
        self.running = False
        if self.arduino:
            self.arduino.close()
        self.root.destroy()

if __name__ == "__main__":
    window = tk.Tk()
    app = SmartHomeApp(window)
    window.mainloop()
