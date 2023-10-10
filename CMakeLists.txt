# Minimum required CMake version
cmake_minimum_required(VERSION 3.12)

# Project name
project(test-project)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Vulkan package
find_package(Vulkan REQUIRED)

# Find GLFW package
find_package(glfw3 REQUIRED)

# Add the ImGui source files
file(GLOB IMGUI_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
list(APPEND EXTRA_SOURCES ${IMGUI_SOURCES})

# Add your project source files
file(GLOB PROJECT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
list(APPEND EXTRA_SOURCES ${PROJECT_SOURCES})

# Create an executable
add_executable(${PROJECT_NAME} ${EXTRA_SOURCES})

# Link against Vulkan, GLFW, and any additional libraries you may need
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan glfw)
