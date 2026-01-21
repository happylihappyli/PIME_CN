import math
import ctypes
import sys

try:
    import tkinter as tk
except ImportError:
    ctypes.windll.user32.MessageBoxW(0, "Error: Tkinter module not found.\nPlease install python-tk or check your Python installation.", "PIME Error", 0x10)
    sys.exit(1)

def calculate(event=None):
    expression = entry.get()
    try:
        # Allow usage of math functions like sin, cos, sqrt, pi, e
        allowed_names = {k: v for k, v in math.__dict__.items() if not k.startswith("__")}
        result = eval(expression, {"__builtins__": None}, allowed_names)
        label_result.config(text=str(result), fg="black")
    except Exception as e:
        label_result.config(text="Error", fg="red")

def on_escape(event=None):
    root.destroy()

root = tk.Tk()
root.title("Calculator")
root.geometry("300x120")
root.resizable(False, False)
root.attributes("-topmost", True) # Keep window on top

# Center the window
root.update_idletasks()
width = 300
height = 120
x = (root.winfo_screenwidth() // 2) - (width // 2)
y = (root.winfo_screenheight() // 2) - (height // 2)
root.geometry(f"{width}x{height}+{x}+{y}")

tk.Label(root, text="Enter expression (e.g. 1+2*3):").pack(pady=5)

entry = tk.Entry(root, font=("Arial", 14))
entry.pack(fill=tk.X, padx=10, pady=5)
entry.bind("<Return>", calculate)
entry.bind("<Escape>", on_escape)
entry.focus_set()

label_result = tk.Label(root, text="", font=("Arial", 14, "bold"))
label_result.pack(pady=5)

root.mainloop()
