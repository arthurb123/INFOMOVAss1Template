@echo off
SETLOCAL

:: Ensure the script starts in the directory where the batch script is located
cd /d "%~dp0"

:: Check if we are in the correct directory that contains both the 'vcpkg' folder and your project's CMakeLists.txt
if not exist "vcpkg" (
    echo Error: vcpkg directory not found
    exit /b
)
if not exist "CMakeLists.txt" (
    echo Error: CMakeLists.txt not found in this directory
    exit /b
)

:: Bootstrap vcpkg (compile vcpkg executable)
if not exist "vcpkg\vcpkg.exe" (
    echo Bootstrapping vcpkg
    vcpkg\bootstrap-vcpkg.bat
)

:: Integrate vcpkg with Visual Studio to make it easier to use with CMake and VS
echo Integrating vcpkg with build systems
vcpkg\vcpkg integrate install

:: Install necessary packages
echo Installing packages
vcpkg\vcpkg install glfw3
vcpkg\vcpkg install opencl
vcpkg\vcpkg install glad
vcpkg\vcpkg install opengl
vcpkg\vcpkg install zlib

:: Configure the project with CMake, specifying the path to the vcpkg toolchain file
echo Configuring CMake project
cmake -B build -S . -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="%CD%\vcpkg\scripts\buildsystems\vcpkg.cmake"

:: Build the project using CMake
echo Building CMake project
cmake --build build --config Release

:: Pause to view output before closing the window
pause

ENDLOCAL
