import ctypes
import os
import subprocess
import time

def get_cached_hwnd():
    try:
        buf = ctypes.create_unicode_buffer(1024)
        ctypes.windll.kernel32.GetTempPathW(1024, buf)
        temp_path = buf.value
        path = os.path.join(temp_path, "pime_calc_hwnd.txt")
        print(f"Checking temp file: {path}")
        if os.path.exists(path):
            with open(path, "r") as f:
                val = f.read().strip()
                print(f"File content: {val}")
                return int(val)
    except Exception as e:
        print(f"Error reading cache: {e}")
    return None

print("Checking for existing calculator...")
hwnd = get_cached_hwnd()
print(f"Cached HWND: {hwnd}")

if hwnd:
    if ctypes.windll.user32.IsWindow(hwnd):
        print("Window is valid. Sending Show message...")
        ctypes.windll.user32.PostMessageW(hwnd, 0x0401, 0, 0)
    else:
        print("Window handle is invalid.")
else:
    print("No cached HWND found.")
    
# Try FindWindow
hwnd_fw = ctypes.windll.user32.FindWindowW("PIMECalculatorWindow", None)
print(f"FindWindow HWND: {hwnd_fw}")

if hwnd_fw:
     ctypes.windll.user32.PostMessageW(hwnd_fw, 0x0401, 0, 0)
