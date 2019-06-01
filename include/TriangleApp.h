#ifndef TRIANGLE_APP_H
#define TRIANGLE_APP_H
#define GLFW_INCLUDE_VULKAN

#include "ExtensionValidation.h"
#include <functional>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>

namespace vulkan_rendering {

    class TriangleApp {

        public:
            TriangleApp();
            void run();

        private:
            // Constants
            const int WIDTH  = 800;
            const int HEIGHT = 600;

            // Ext validation
            ExtensionValidation ext_validation;

            // Variables
            GLFWwindow* window;
            VkInstance instance;

            // Functions
            void init_window();
            void init_vulkan();
            void main_loop();
            void cleanup();

            // Vulkan
            void create_instance();
    };
}

#endif
