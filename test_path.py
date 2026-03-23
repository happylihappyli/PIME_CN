import os
import sys

# 模拟 rime_ime.py 的位置
current_dir = r"e:\GitHub3\cpp\PIME_CN\python\input_methods\rime"

print(f"Current dir (simulated): {current_dir}")

cpp_calc_scons = os.path.normpath(os.path.join(current_dir, "../../../CppCalculator/CppCalculator.exe"))
cpp_calc_cmake = os.path.normpath(os.path.join(current_dir, "../../../CppCalculator/build/Release/CppCalculator.exe"))

print(f"Checking SCons path: {cpp_calc_scons}")
print(f"Exists: {os.path.exists(cpp_calc_scons)}")

print(f"Checking CMake path: {cpp_calc_cmake}")
print(f"Exists: {os.path.exists(cpp_calc_cmake)}")
