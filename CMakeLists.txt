cmake_minimum_required(VERSION 3.18...3.27.8)

option(USE_ARM "Use ARM architecture" OFF)

project(INFOMOV VERSION 1.0
        DESCRIPTION "Exercises for the INFOMOV lecture"
        LANGUAGES CXX)
    
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Setup executable
add_executable(
    GAME game.cpp
    template/opencl.cpp
    template/opengl.cpp
    template/sprite.cpp
    template/surface.cpp
    template/template.cpp
    template/tmpl8math.cpp
)

# ARM specific code
if (USE_ARM)
    target_compile_definitions(GAME PRIVATE USE_ARM)

    add_subdirectory("arm/cpuid" cpuid)
    target_link_libraries(GAME PUBLIC steinwurf::cpuid)
endif()

# Find and link OpenCL
find_package(OpenCL REQUIRED)
target_link_libraries(GAME PUBLIC OpenCL::OpenCL)

# Find and link OpenGL
find_package(OpenGL REQUIRED)
target_link_libraries(GAME PRIVATE OpenGL::GL)

# Find and link GLAD
find_package(glad CONFIG REQUIRED)
target_link_libraries(GAME PRIVATE glad::glad)

# Find and link zlib
find_package(ZLIB REQUIRED)
target_link_libraries(GAME PUBLIC ZLIB::ZLIB)

# Find and link GLFW
find_package(glfw3 REQUIRED)
target_link_libraries(GAME PUBLIC glfw)

# If specific additional libraries are needed (like m and dl on Unix-based systems), use conditional linking
include(CheckLibraryExists)
check_library_exists(m pow "" HAVE_LIB_M)
if(HAVE_LIB_M)
    target_link_libraries(GAME PUBLIC m)
endif()

check_library_exists(dl dlopen "" HAVE_LIB_DL)
if(HAVE_LIB_DL)
    target_link_libraries(GAME PUBLIC dl)
endif()

# Include directories
target_include_directories(GAME PUBLIC template)

# Precompile the precomp.h
target_precompile_headers(GAME PUBLIC template/precomp.h)

# Copy assets
add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets
    COMMENT "Copying assets to build directory"
)

# Ensure that your main target triggers the asset copy
add_dependencies(GAME copy_assets)
