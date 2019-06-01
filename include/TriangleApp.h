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

            // static calls
            static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
                VkDebugUtilsMessageSeverityFlagBitsEXT msg_severity,
                VkDebugUtilsMessageTypeFlagsEXT msg_type, const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
                void* p_user_data) {

                std::cerr << "Validation Layer: " << p_callback_data->pMessage << std::endl;
                return VK_FALSE;
            }

        private:
            // Constants
            const int WIDTH  = 800;
            const int HEIGHT = 600;
            const std::vector<const char*> validation_layers = { "VK_LAYER_KHRONOS_validation" };

            #if NDEBUG
            const bool enable_validation_layers = false;
            #else
            const bool enable_validation_layers = true;
            #endif

            // Ext validation
            ExtensionValidation ext_validation;

            // Variables
            GLFWwindow* window;
            VkInstance instance;
            VkDebugUtilsMessengerEXT debug_messenger;

            // Functions
            void init_window();
            void init_vulkan();
            void main_loop();
            void cleanup();

            // Vulkan
            void create_instance();
            bool check_validation_layers_support();
            std::vector<const char*> get_required_extensions();

            // Debug
            void setup_debug_messenger();
            void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info);
    };
}

#endif
