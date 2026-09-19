
@ pip install SpeechRecognition pyttsx3 pyaudio
# pip install ultralytics opencv-python requests pyserial SpeechRecognition pyttsx3 pyaudio pillow

# (Note: If you face an error installing pyaudio, you can install it easily on Windows by running pip install pipwin followed by pipwin install pyaudio).



import os
import subprocess
import sys
import webbrowser
import requests
import speech_recognition as sr
import pyttsx3

# Configuration Paths
YOUTUBE_PLAYLIST = "https://youtube.com"
OFFLINE_MUSIC_DIR = r"C:\Music"  # Folder where your offline Christian MP3s are saved
VLC_PATH = r"C:\Program Files\VideoLAN\VLC\vlc.exe"  # Standard installation path for VLC

# ------------------------------------------------------------------------------------

# Initialize Voice
engine = pyttsx3.init()
engine.setProperty('rate', 150)

def speak(text):
    print(f"🤖 Speaking: {text}")
    engine.say(text)
    engine.runAndWait()

def is_internet_working():
    """Quickly check if internet is available by pinging Google DNS."""
    try:
        requests.get("https://1.1.1", timeout=2)
        return True
    except (requests.ConnectionError, requests.Timeout):
        return False

def launch_music():
    """Decides whether to play online via YouTube or offline via VLC."""
    if is_internet_working():
        speak("Internet is active. Opening your Christian songs playlist on YouTube.")
        webbrowser.open(YOUTUBE_PLAYLIST)
    else:
        speak("Internet is offline. Redirecting to VLC local storage playback mode.")
        
        # Check if the user's music folder actually exists
        if os.path.exists(OFFLINE_MUSIC_DIR):
            # Command to launch VLC and tell it to play the entire folder randomly
            vlc_command = [VLC_PATH, OFFLINE_MUSIC_DIR, "--random", "--loop"]
            
            try:
                # Use subprocess to fire up VLC silently in the background
                subprocess.Popen(vlc_command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            except FileNotFoundError:
                speak("Error: VLC Media Player was not found at the standard directory path.")
        else:
            speak("Error: No offline tracks found at C drive Music folder.")

def listen_for_command():
    recognizer = sr.Recognizer()
    with sr.Microphone() as source:
        print("\n🎙️ Listening...")
        recognizer.adjust_for_ambient_noise(source, duration=0.5)
        try:
            audio = recognizer.listen(source, timeout=4, phrase_time_limit=2)
            command = recognizer.recognize_google(audio).lower()
            print(f"🗣️ Heard: '{command}'")
            return command
        except Exception:
            return ""

# Execution Loop
speak("Voice engine initialized.")
while True:
    voice_input = listen_for_command()
    if "play" in voice_input:
        launch_music()
        break
