#!/bin/bash

# Ensure the script starts in the directory where the script is located
cd "$(dirname "$0")"

# Check if we are in the correct directory that contains both the 'vcpkg' folder and the project's CMakeLists.txt
if [ ! -d "vcpkg" ]; then
    echo "Error: vcpkg directory not found"
    exit 1
fi

if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found in this directory"
    exit 1
fi

# Bootstrap vcpkg (compile vcpkg executable)
if [ ! -f "vcpkg/vcpkg" ]; then
    echo "Bootstrapping vcpkg"
    ./vcpkg/bootstrap-vcpkg.sh
fi

# Integrate vcpkg with build systems to make it easier to use with CMake
echo "Integrating vcpkg with build systems"
./vcpkg/vcpkg integrate install

# Install necessary packages
echo "Installing packages"
./vcpkg/vcpkg install glfw3
./vcpkg/vcpkg install opencl
./vcpkg/vcpkg install glad
./vcpkg/vcpkg install opengl
./vcpkg/vcpkg install zlib

# Configure the project with CMake, specifying the path to the vcpkg toolchain file
echo "Configuring CMake project"
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="$(pwd)/vcpkg/scripts/buildsystems/vcpkg.cmake"  -DUSE_ARM=OFF

# Build the project using CMake
echo "Building CMake project"
cmake --build build --config Release

# Allow user to see output before closing the terminal
read -p "Press any key to continue..."