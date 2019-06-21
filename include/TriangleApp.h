#ifndef TRIANGLE_APP_H
#define TRIANGLE_APP_H
#define GLFW_INCLUDE_VULKAN

#include "ExtensionValidation.h"
#include "QueueFamilyIndices.h"
#include "SwapChainSupportDetails.h"
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
            const std::vector<const char*> validation_layers = { "VK_LAYER_KHRONOS_validation" };
            const std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

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
            VkPhysicalDevice physical_device = VK_NULL_HANDLE;
            VkDevice device;
            VkQueue graphics_queue;
            VkSurfaceKHR surface;
            VkQueue present_queue;
            VkSwapchainKHR swap_chain;
            std::vector<VkImage> swap_chain_images;
            VkFormat swap_chain_image_format;
            VkExtent2D swap_chain_extent;
            std::vector<VkImageView> swap_chain_image_views;
            VkPipelineLayout pipeline_layout;
            VkRenderPass render_pass;
            VkPipeline graphics_pipeline;

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

            // Device selection
            void pick_physical_device();
            bool is_device_suitable(VkPhysicalDevice device);
            QueueFamilyIndices find_queue_families(VkPhysicalDevice device);
            void create_logical_device();
            void create_surface();
            bool check_device_extension_support(VkPhysicalDevice device);
            SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device);
            VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats);
            VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes);
            VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);
            void create_swap_chain();
            void create_image_views();
            void create_graphics_pipeline();
            VkShaderModule create_shader_module(const std::vector<char>& code);
            void create_render_pass();
    };
}

#endif
