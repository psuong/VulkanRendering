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
            bool frame_buffer_resized_flag = false;

            TriangleApp();
            void run();

        private:
            // Constants
            const int WIDTH  = 800;
            const int HEIGHT = 600;
            const std::vector<const char*> validation_layers = { "VK_LAYER_KHRONOS_validation" };
            const std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
            const int max_frames_per_flight = 2;

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
            VkDescriptorSetLayout descriptor_set_layout; // newly added
            VkPipelineLayout pipeline_layout;
            VkRenderPass render_pass;
            VkPipeline graphics_pipeline;
            std::vector<VkFramebuffer> swap_chain_frame_buffers;
            VkCommandPool command_pool;
            std::vector<VkCommandBuffer> command_buffers;

            std::vector<VkSemaphore> img_available_semaphores;
            std::vector<VkSemaphore> render_finished_semaphores;
            std::vector<VkFence> flight_fences;
            size_t current_frame = 0;
            VkBuffer vertex_buffer;
            VkDeviceMemory vertex_buffer_memory;
            VkBuffer index_buffer;
            VkDeviceMemory index_buffer_memory;

            /**
             * The whole point of this is to support what happens if we have multiple frames in flight. While we can use 
             * use a staging buffer, we're going to be updating the buffer every frame. So the approach is to store
             * multiple buffers at the same time. If we had one buffer, we dont want to change the buffer while another 
             * frame is still reading it, so we "queue" up these frames.
             */
            std::vector<VkBuffer> uniform_buffers;
            std::vector<VkDeviceMemory> uniform_buffers_memory;

            // Functions
            void init_window();
            void init_vulkan();
            void main_loop();
            void cleanup();
            void cleanup_swap_chain();

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
            void create_frame_buffers();
            void create_command_pool();
            void create_command_buffers();

            // Let the drawing begin!
            void draw_frame();
            void create_sync_objects();
            void recreate_swap_chain();
            void create_vertex_buffer();
            uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags props);

            void create_buffer(VkDeviceSize size, VkBufferUsageFlags flags, VkMemoryPropertyFlags props, 
                VkBuffer& buffer, VkDeviceMemory& buffer_mem);
            void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

            void create_index_buffer();
            void create_descriptor_set_layout();
            void create_uniform_buffers();
    };
}

#endif
