cmake_minimum_required(VERSION 3.5)

project(vulkan-rendering)

set(BIN_NAME "vk-rendering")

set(SOURCES
    FileHelper.h
    SwapChainSupportDetails.h
    QueueFamilyDevice.h
    TriangleApp.h
    main.cpp
    TriangleApp.cpp)

include_directories("$ENV{VULKAN_SDK}/include")
link_directories("$ENV{VULKAN_SDK}/lib") 
link_directories("$ENV{VULKAN_SDK}/etc/explicit_layer.d")

find_package(glfw3 3.3 REQUIRED)

set(CMAKE_CXX_STANDARD 17)
add_executable(${BIN_NAME} ${SOURCES})
target_link_libraries(${BIN_NAME} glfw)
target_link_libraries(${BIN_NAME} vulkan)
