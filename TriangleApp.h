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

    // Extensions function to create the debuging messenger
    static VkResult Create_Debug_Utils_Messenger(
        VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    static void Destroy_Debug_Utils_Messenger(
        VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }

    class TriangleApp {

    public:
        void run();

    private:
        const int WIDTH = 800;
        const int HEIGHT = 600;
        const std::vector<const char*> validation_layers = {
            "VK_LAYER_LUNARG_standard_validation"
        };

        const std::vector<const char*> device_extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        VkPhysicalDevice physical_device;
        VkDevice device;
        VkQueue graphics_queue;
        VkQueue present_queue;
        VkSurfaceKHR surface;
        VkSwapchainKHR swap_chain;
        std::vector<VkImage> swap_chain_images;
        VkFormat swap_chain_image_format;
        VkExtent2D swap_chain_extent;

        std::vector<VkImageView> swapchain_image_views;

        VkRenderPass render_pass;
        VkPipelineLayout pipeline_layout;
        VkPipeline graphics_pipeline;

        // Frame Buffers
        std::vector<VkFramebuffer> swapchain_frame_buffers;

        // Command Buffer
        VkCommandPool command_pool;

#if NDEBUG
        const bool enable_validation_layers = false;
#else
        const bool enable_validation_layers = true;
#endif

        GLFWwindow* window;
        VkInstance instance;
        VkDebugUtilsMessengerEXT debug_messenger;

        // Functions
        void init_window();
        void init_vulkan();
        void main_loop();
        void cleanup();
        void setup_debugger();
        void select_physical_device(std::function<bool(VkPhysicalDevice)> validation);
        void create_swap_chain();
        void inline create_instance();
        void inline create_logical_device();
        void inline create_surface();
        bool inline check_validation_support();
        bool inline check_device_extension_support(VkPhysicalDevice device);
        std::vector<const char*> get_required_extensions();
        QueueFamilyDevice queue_families(VkPhysicalDevice device);
        SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device);
        VkSurfaceFormatKHR select_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats);
        VkPresentModeKHR select_presentation_mode(const std::vector<VkPresentModeKHR>& available_presentations);
        VkExtent2D select_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);
        void create_render_pass();

        // Image functions
        void create_image_views();
        void create_graphics_pipeline();
        VkShaderModule create_shader_module(const std::vector<char>& code);

        void create_frame_buffers();
        void create_command_pool();

        static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData);
    };
}

#endif
