import ctypes
import os
import sys

dll_path = os.path.abspath(r"build\PIMETextService.dll")
print(f"Trying to load: {dll_path}")

try:
    # Try using LoadLibrary directly to avoid some python path issues
    lib = ctypes.windll.LoadLibrary(dll_path)
    print("Load successful!")
except OSError as e:
    print(f"Load failed: {e}")
    if hasattr(e, 'winerror'):
        print(f"WinError: {e.winerror}")
