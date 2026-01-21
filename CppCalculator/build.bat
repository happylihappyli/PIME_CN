@echo off
mkdir build
cd build
cmake ..
cmake --build . --config Release
cd ..
echo Build complete. Executable is in build\Release\CppCalculator.exe
