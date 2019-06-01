#define GLFW_INCLUDE_VULKAN

#include "TriangleApp.h"
#include <algorithm>
#include <GLFW/glfw3.h>
#include <iostream>
#include <set>
#include <string.h>
#include <vulkan/vulkan.h>

namespace vulkan_rendering {

    void TriangleApp::run() {
        init_window();
        init_vulkan();
        main_loop();
        cleanup();
    }

    void TriangleApp::init_window() {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }
}
