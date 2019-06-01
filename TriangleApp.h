#ifndef TRIANGLE_APP_H
#define TRIANGLE_APP_H
#define GLFW_INCLUDE_VULKAN

#include "QueueFamilyDevice.h"
#include "SwapChainSupportDetails.h"
#include <functional>
#include <GLFW/glfw3.h>
#include <vector>
#include <vulkan/vulkan.h>

namespace vulkan_rendering {

    class TriangleApp {

        public:
            void run();

        private:
            void init_window();
            void init_vulkan();
            void main_loop();
            void cleanup();
    };
}

#endif
