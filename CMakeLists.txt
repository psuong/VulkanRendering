cmake_minimum_required(VERSION 3.5)

project(vulkan-rendering)

set(BIN_NAME "vk-rendering")

set(SOURCES
    include/ExtensionValidation.h
    include/TriangleApp.h
    include/QueueFamilyIndices.h
    src/main.cpp
    src/ExtensionValidation.cpp
    src/TriangleApp.cpp)

include_directories("$ENV{VULKAN_SDK}/include")
link_directories("$ENV{VULKAN_SDK}/lib") 
link_directories("$ENV{VULKAN_SDK}/etc/explicit_layer.d")

find_package(glfw3 3.3 REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
add_executable(${BIN_NAME} ${SOURCES})
target_link_libraries(${BIN_NAME} glfw)
target_link_libraries(${BIN_NAME} vulkan)
