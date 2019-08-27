#define GLFW_INCLUDE_VULKAN

#include "../include/ExtensionValidation.h"
#include "../include/QueueFamilyIndices.h"
#include "../include/SwapChainSupportDetails.h"
#include "../include/TriangleApp.h"
#include "../include/FileHelper.h"
#include "../include/Vertex.h"

#include <algorithm>
#include <GLFW/glfw3.h>
#include <iostream>
#include <set>
#include <string.h>
#include <vulkan/vulkan.h>

namespace vulkan_rendering {

    /**
     * Like delegates in C#, we look up the address of the function and actually invoke it. This is an abstracted
     * "proxy" to do said operation.
     */
    VkResult create_debug_utils_messenger_ext(
        VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* p_create_info, 
        const VkAllocationCallbacks* p_allocator, VkDebugUtilsMessengerEXT* p_debug_messenger) {

        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, p_create_info, p_allocator, p_debug_messenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    /**
     * Need to clean up the debugger when ending our process. Vulkan is a very memory manual managed API.
     */
    void destroy_debug_utils_messenger_ext(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, 
        const VkAllocationCallbacks* p_allocator) {

        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, 
            "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debug_messenger, p_allocator);
        }
    }



    /**
     * VKAPI_ATTR and VKAPI_CALL would ensures that callback called has the right signature for Vulkan to call it.
     * We want to look through the callback and print out the validation layer's message.
     */
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, 
        const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_user_data) {
        std::cerr << "Validation Layer: " << p_callback_data->pMessage << std::endl;
        return VK_FALSE;
    }

    static void frame_buffer_resize_callback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<TriangleApp*>(glfwGetWindowUserPointer(window));
        app->frame_buffer_resized_flag = true;
    }

    TriangleApp::TriangleApp() {
        ext_validation = ExtensionValidation();
    }

    void TriangleApp::run() {
        init_window();
        init_vulkan();
        main_loop();
        cleanup();
    }

    void TriangleApp::init_window() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, frame_buffer_resize_callback);
    }

    void TriangleApp::init_vulkan() {
        create_instance();
        setup_debug_messenger();
        create_surface();
        pick_physical_device();
        create_logical_device();
        create_swap_chain();
        create_image_views();
        create_render_pass();
        create_graphics_pipeline();
        create_frame_buffers();
        create_command_pool();
        create_vertex_buffer();
        create_command_buffers();
        create_sync_objects();
    }

    void TriangleApp::main_loop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            draw_frame();
        }

        // TODO: Check this...
        // vkQueuePresentKHR(present_queue, present_info);

        /**
         * Wait for the seamphore to be finished in the frame buffers buffer exiting
         */
        vkDeviceWaitIdle(device);
    }

    void TriangleApp::cleanup() {
        cleanup_swap_chain();

        vkDestroyBuffer(device, vertex_buffer, nullptr);
        vkFreeMemory(device, vertex_buffer_memory, nullptr);

        for (int i = 0; i < max_frames_per_flight; i++) {
            vkDestroySemaphore(device, render_finished_semaphores[i], nullptr);
            vkDestroySemaphore(device, img_available_semaphores[i], nullptr);
            vkDestroyFence(device, flight_fences[i], nullptr);
        }

        vkDestroyCommandPool(device, command_pool, nullptr);
        vkDestroyDevice(device, nullptr);

        if (enable_validation_layers) {
            destroy_debug_utils_messenger_ext(instance, debug_messenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void TriangleApp::cleanup_swap_chain() {
        for (size_t i = 0; i < swap_chain_frame_buffers.size(); i++) {
            vkDestroyFramebuffer(device, swap_chain_frame_buffers[i], nullptr);
        }

        vkFreeCommandBuffers(device, command_pool, static_cast<uint32_t>(command_buffers.size()),
            command_buffers.data());

        vkDestroyPipeline(device, graphics_pipeline, nullptr);
        vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
        vkDestroyRenderPass(device, render_pass, nullptr);

        for (size_t i = 0; i < swap_chain_image_views.size(); i++) {
            vkDestroyImageView(device, swap_chain_image_views[i], nullptr);
        }

        vkDestroySwapchainKHR(device, swap_chain, nullptr);
    }
    
    // Vulkan functions
    /*
     * Constucts the Vulkan instances with all the required extensions.
     */
    void TriangleApp::create_instance() {
        if (enable_validation_layers && !check_validation_layers_support()) {
            throw std::runtime_error("Validation layers requested, but not available");
        }

        VkApplicationInfo app_info  = {};
        app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName   = "Hello Triangle";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName        = "No engine";
        app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion         = VK_API_VERSION_1_0;

        VkInstanceCreateInfo create_info = {};
        create_info.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo     = &app_info;

        auto extensions = get_required_extensions();
        create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
        /**
         * We can enable the validation layers and we want to include them into the struct on Vulkan creation.
         */
        if (enable_validation_layers) {
            create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
            create_info.ppEnabledLayerNames = validation_layers.data();

            populate_debug_messenger_create_info(debug_create_info);
            create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
        } else {
            create_info.enabledLayerCount = 0;
            create_info.pNext = nullptr;
        }

        if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create instance!");
        }

        // TODO: Add the extension validation
    }

    /**
     * Ensure that we have validation layers support in our instance.
     */
    bool TriangleApp::check_validation_layers_support() {
        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        for (const char* layer_name : validation_layers) {
            bool layer_found = false;

            for (const auto& layer_props : available_layers) {
                if (strcmp(layer_name, layer_props.layerName) == 0) {
                    layer_found = true;
                    break;
                }
            }

            if (!layer_found) {
                return false;
            }
        }

        return true;
    }

    /**
     * So enabling validations is all fun, but there needs to be a way to retrieve those messages to relay back. 
     * Otherwise debugging is pretty useful. Since I'm using GLFW, I need the required GLFW extensions first.
     */
    std::vector<const char*> TriangleApp::get_required_extensions() {
        uint32_t glfw_extension_count = 0;
        const char** glfw_extensions;
        glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

        if (enable_validation_layers) {
            // Macro here which is equivalent to: VK_EXT_debug_utils
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    /**
     * Create the debug messenger and populate the messenger if we allow validation layers.
     */
    void TriangleApp::setup_debug_messenger() {
        if (!enable_validation_layers) return;

        VkDebugUtilsMessengerCreateInfoEXT create_info;
        populate_debug_messenger_create_info(create_info);

        if (create_debug_utils_messenger_ext(instance, &create_info, nullptr, &debug_messenger) != VK_SUCCESS) {
            throw std::runtime_error("Failed to set up debug messenger!");
        }
    }

    /**
     * Populate the debug messenger with the right flags.
     */
    void TriangleApp::populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info) {
        create_info                 = {};
        create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        create_info.pfnUserCallback = debug_callback;
    }

    /**
     * Physical Device Selection
     * List all devices available by querying the amount first. Then query all the actual devices and check if the 
     * physical device supports what we need :). Then we can store it into our physical_device variable.
     */
    void TriangleApp::pick_physical_device() {
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

        if (device_count == 0) {
            throw std::runtime_error("Failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

        for (const auto& device : devices) {
            if (is_device_suitable(device)) {
                physical_device = device;
                break;
            }
        }

        if (physical_device == VK_NULL_HANDLE) {
            throw std::runtime_error("Failed to find a suitable GPU!");
        }
    }

    /**
     * Check if the device actually fits our needs, that's really it.
     */
    bool TriangleApp::is_device_suitable(VkPhysicalDevice device) {
        QueueFamilyIndices indices = find_queue_families(device);

        bool extension_support = check_device_extension_support(device);
        bool swap_chain_adequate = false;

        if (extension_support) {
            SwapChainSupportDetails swap_chain_support = query_swap_chain_support(device);
            swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
        }

        return indices.is_complete() && extension_support && swap_chain_adequate;
    }

    /**
     * Queueing families work the same way as extensions. Find the number of devices and check if each queried device 
     * can support VK_QUEUE_GRAPHICS_BIT when it supports it, add it to the QueeuFamilyIndices struct and break out of 
     * the loop. In the future, we can make this much more complex.
     */
    QueueFamilyIndices TriangleApp::find_queue_families(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

        int i = 0;
        for (const auto& queue_family : queue_families) {
            if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphics_family = i; 
            }

            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

            if (queue_family.queueCount > 0 && present_support) {
                indices.present_family = i;
            }

            if (indices.is_complete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    void TriangleApp::create_logical_device() {
        QueueFamilyIndices indices = find_queue_families(physical_device);

        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        std::set<uint32_t> unique_queue_families = { indices.graphics_family.value(), indices.present_family.value() };

        float queue_priority = 1.0f;
        for (uint32_t queue_family : unique_queue_families) {
            VkDeviceQueueCreateInfo queue_create_info = {};
            queue_create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueCount              = 1;
            queue_create_info.queueFamilyIndex        = queue_family;
            queue_create_info.pQueuePriorities        = &queue_priority;
            queue_create_infos.push_back(queue_create_info);
        }

        VkPhysicalDeviceFeatures device_features = {};

        VkDeviceCreateInfo create_info = {};
        create_info.sType              = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
        create_info.pQueueCreateInfos    = queue_create_infos.data();

        create_info.pEnabledFeatures = &device_features;

        create_info.enabledExtensionCount   = static_cast<uint32_t>(device_extensions.size());
        create_info.ppEnabledExtensionNames = device_extensions.data();

        if (enable_validation_layers) {
            create_info.enabledLayerCount   = static_cast<uint32_t>(validation_layers.size());
            create_info.ppEnabledLayerNames = validation_layers.data();
        } else {
            create_info.enabledLayerCount   = 0;
        }

        if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create logical device!");
        }

        vkGetDeviceQueue(device, indices.graphics_family.value(), 0, &graphics_queue);
        vkGetDeviceQueue(device, indices.present_family.value(), 0, &present_queue);
    }

    void TriangleApp::create_surface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create window surface!");
        }
    }

    bool TriangleApp::check_device_extension_support(VkPhysicalDevice device) {
        uint32_t extension_count;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

        std::vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

        std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

        for (const auto& extension : available_extensions) {
            required_extensions.erase(extension.extensionName);
        }

        return required_extensions.empty();
    }

    SwapChainSupportDetails TriangleApp::query_swap_chain_support(VkPhysicalDevice device) {
        SwapChainSupportDetails details; 
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
        uint32_t format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

        if (format_count != 0) {
            details.formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
        }

        uint32_t present_mode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

        if (present_mode_count != 0) {
            details.present_modes.resize(present_mode_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data());
        }

        return details;
    }

    VkSurfaceFormatKHR TriangleApp::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats) {
        
        // This is the best case scenario, we have no formats specified
        if (available_formats.size() == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED) {
            // UNORM is the most common format we want to work with, we're not working with SRGB format.
            return { VK_FORMAT_B8G8R8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
        }

        // Check for a preferred combination
        for (const auto& available_format : available_formats) {
            if (available_format.format == VK_FORMAT_B8G8R8_UNORM && 
                available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return available_format;
            }
        }

        // Default to the last one if no matches work
        return available_formats[0];
    }

    VkPresentModeKHR TriangleApp::choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes) {
        for (const auto& available_present_mode : available_present_modes) {
            if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return available_present_mode;
            } else if (available_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                return available_present_mode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D TriangleApp::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            /**
             * If we resize the window we need to grab the size of the window again...
             */
            // TODO: Handle suboptimal or out of date swap chains
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            VkExtent2D actual_extent = { (uint32_t)width, (uint32_t)height };

            actual_extent.width = std::max(capabilities.minImageExtent.width, 
                std::min(capabilities.maxImageExtent.width, actual_extent.width));
            actual_extent.height = std::max(capabilities.minImageExtent.height, 
                std::min(capabilities.maxImageExtent.height, actual_extent.height));

            return actual_extent;
        }
    }

    void TriangleApp::create_swap_chain() {
        SwapChainSupportDetails swap_chain_support = query_swap_chain_support(physical_device);
        VkSurfaceFormatKHR surface_format          = choose_swap_surface_format(swap_chain_support.formats);
        VkPresentModeKHR present_mode              = choose_swap_present_mode(swap_chain_support.present_modes);
        VkExtent2D extent                          = choose_swap_extent(swap_chain_support.capabilities);

        // If we stick with the minimum number of images, then we yield until the internal operations are done
        // so we request a secondary image count
        uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;

        // We want to clamp the image_count to the max amt of images we can support
        if (swap_chain_support.capabilities.maxImageCount > 0 && image_count >
            swap_chain_support.capabilities.maxImageCount) {
            image_count = swap_chain_support.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR create_info = {};
        create_info.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface                  = surface;

        /**
         * We want to render directly so we use the VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, but we can use a separate
         * image to render to first so that we can apply post processing: 
         * VK_IMAGE_USAGE_TRANSFER_DST_BIT
         * We can then perform a mem operation to transfer the rendered img to a swap chain image.
         */
        create_info.minImageCount    = image_count;
        create_info.imageFormat      = surface_format.format;
        create_info.imageColorSpace  = surface_format.colorSpace;
        create_info.imageExtent      = extent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; 

        QueueFamilyIndices indices      = find_queue_families(physical_device);
        uint32_t queue_family_indices[] = { indices.graphics_family.value(), indices.present_family.value() };

        if (indices.graphics_family != indices.present_family) {
            create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = 2;
            create_info.pQueueFamilyIndices   = queue_family_indices;
        } else {
            create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
            create_info.queueFamilyIndexCount = 0;
            create_info.pQueueFamilyIndices   = nullptr;
        }

        create_info.preTransform   = swap_chain_support.capabilities.currentTransform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.presentMode    = present_mode;
        create_info.clipped        = VK_TRUE;

        // We assume there's only one swap chain, but swap chains can be recreated when you resize the window.
        create_info.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &create_info, nullptr, &swap_chain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(device, swap_chain, &image_count, nullptr);
        swap_chain_images.resize(image_count);
        vkGetSwapchainImagesKHR(device, swap_chain, &image_count, swap_chain_images.data());
        
        // Store the swap chain's formats and extents
        swap_chain_image_format = surface_format.format;
        swap_chain_extent       = extent;
    }

    void TriangleApp::create_image_views() {
        swap_chain_image_views.resize(swap_chain_images.size());

        for (size_t i = 0; i < swap_chain_images.size(); i++) {
            VkImageViewCreateInfo create_info = {};
            create_info.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            create_info.image                 = swap_chain_images[i];
            
            // Specify how we should interpret the data.
            create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            create_info.format = swap_chain_image_format;

            // We can swizzle or use constants.
            create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            // Define the image subresources and yo ucan create multiple image layers for stereogaphics 3d apps.
            create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            create_info.subresourceRange.baseMipLevel   = 0;
            create_info.subresourceRange.levelCount     = 1;
            create_info.subresourceRange.baseArrayLayer = 0;
            create_info.subresourceRange.layerCount     = 1;

            if (vkCreateImageView(device, &create_info, nullptr, &swap_chain_image_views[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create image view");
            }
        }
    }

    void TriangleApp::create_graphics_pipeline() {
        auto vert_shader_code = read_file("/home/psuong/Documents/Projects/VulkanRendering/shaders/vert.spv");
        auto frag_shader_code = read_file("/home/psuong/Documents/Projects/VulkanRendering/shaders/frag.spv");

        VkShaderModule vert_shader_module = create_shader_module(vert_shader_code);
        VkShaderModule frag_shader_module = create_shader_module(frag_shader_code);

        VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
        vert_shader_stage_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vert_shader_stage_info.stage                           = VK_SHADER_STAGE_VERTEX_BIT;
        vert_shader_stage_info.module                          = vert_shader_module;
        vert_shader_stage_info.pName                           = "main";

        VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
        frag_shader_stage_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_shader_stage_info.stage                           = VK_SHADER_STAGE_FRAGMENT_BIT;
        frag_shader_stage_info.module                          = frag_shader_module;
        frag_shader_stage_info.pName                           = "main";

        VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_stage_info };

        VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
        vertex_input_info.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto binding_description = Vertex::get_binding_descriptions();
        auto attribute_description = Vertex::get_attribute_descriptions();

        vertex_input_info.vertexBindingDescriptionCount        = 1;
        vertex_input_info.vertexAttributeDescriptionCount      = static_cast<uint32_t>(attribute_description.size());
        vertex_input_info.pVertexBindingDescriptions           = &binding_description;
        vertex_input_info.pVertexAttributeDescriptions         = attribute_description.data();

        // Below is useless when we have vertex bindings available.
        /*
        vertex_input_info.vertexBindingDescriptionCount        = 0;
        vertex_input_info.pVertexBindingDescriptions           = nullptr;
        vertex_input_info.vertexAttributeDescriptionCount      = 0;
        vertex_input_info.pVertexAttributeDescriptions         = nullptr;
        */

        VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
        input_assembly.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly.topology                               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly.primitiveRestartEnable                 = VK_FALSE;

        VkViewport view_port = {};
        view_port.x          = 0.0f;
        view_port.y          = 0.0f;
        view_port.width      = (float) swap_chain_extent.width;
        view_port.height     = (float) swap_chain_extent.height;
        view_port.minDepth   = 0.0f;
        view_port.maxDepth   = 1.0f;

        VkRect2D scissor = {};
        scissor.offset   = { 0, 0 };
        scissor.extent   = swap_chain_extent;

        VkPipelineViewportStateCreateInfo view_port_state = {};
        view_port_state.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        view_port_state.viewportCount                     = 1;
        view_port_state.pViewports                        = &view_port;
        view_port_state.scissorCount                      = 1;
        view_port_state.pScissors                         = &scissor;

        /**
         * If we enable depth clamp, then any fragments beyond the near and far planes are clamped to them instead of 
         * just not rendering them.
         *
         * By enabling the discardEnable property, geometry doesn't pass through the rasterizer stage.
         */
        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable                       = VK_FALSE;
        rasterizer.rasterizerDiscardEnable                = VK_FALSE;
        rasterizer.polygonMode                            = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth                              = 1.0f;

        /**
         * We can cull the front, back or even both. Similarly, we can determine whether something is front facing by 
         * looking at the vertices and determining that they are clockwise.
         */
        rasterizer.cullMode  = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

        /**
         * Depth can be used for shadow maps? Need to look into this.
         */
        rasterizer.depthBiasEnable         = VK_FALSE;

        /**
         * Multi sampling allows for smoother edges along the edges.
         */
        VkPipelineMultisampleStateCreateInfo multi_sampling = {};
        multi_sampling.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multi_sampling.sampleShadingEnable                  = VK_FALSE;
        multi_sampling.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;

        /**
         * Not enabling Depth/Stencil testing
         */

        /**
         * Colour blending allows colours to interpolate somehow between the new colour and the one already in the frame
         * buffer.
         */
        VkPipelineColorBlendAttachmentState color_blend_attachment = {};
        color_blend_attachment.colorWriteMask                      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        color_blend_attachment.blendEnable                         = VK_FALSE;
        color_blend_attachment.srcColorBlendFactor                 = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_attachment.dstColorBlendFactor                 = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_attachment.colorBlendOp                        = VK_BLEND_OP_ADD;
        color_blend_attachment.srcAlphaBlendFactor                 = VK_BLEND_FACTOR_ONE;
        color_blend_attachment.dstAlphaBlendFactor                 = VK_BLEND_FACTOR_ZERO;
        color_blend_attachment.alphaBlendOp                        = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo color_blending = {};
        color_blending.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blending.logicOpEnable                       = VK_FALSE;
        color_blending.logicOp                             = VK_LOGIC_OP_COPY;
        color_blending.attachmentCount                     = 1;
        color_blending.pAttachments                        = &color_blend_attachment;
        color_blending.blendConstants[0]                   = 0.0f;
        color_blending.blendConstants[1]                   = 1.0f;
        color_blending.blendConstants[2]                   = 2.0f;
        color_blending.blendConstants[3]                   = 3.0f;

        /**
         * Maybe add a dynamic state instead without recreating the entire pipeline. E.g viewport, line width, and blend 
         * constants can change over time.
         * TODO: Revisit.
         */

        VkPipelineLayoutCreateInfo pipeline_layout_info = {};
        pipeline_layout_info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount             = 0;
        pipeline_layout_info.pSetLayouts                = nullptr;
        pipeline_layout_info.pushConstantRangeCount     = 0;
        pipeline_layout_info.pPushConstantRanges        = nullptr;

        if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipeline_info = {};
        pipeline_info.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount                   = 2;
        pipeline_info.pStages                      = shader_stages;

        pipeline_info.pVertexInputState   = &vertex_input_info;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState      = &view_port_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState   = &multi_sampling;
        pipeline_info.pDepthStencilState  = nullptr;
        pipeline_info.pColorBlendState    = &color_blending;
        pipeline_info.pDynamicState       = nullptr;

        pipeline_info.layout     = pipeline_layout;
        pipeline_info.renderPass = render_pass;
        pipeline_info.subpass    = 0;

        pipeline_info.basePipelineIndex = VK_NULL_HANDLE;
        pipeline_info.basePipelineIndex = -1;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline) !=
            VK_SUCCESS) {
            throw std::runtime_error("Failed to create graphics pipeline");
        }

        vkDestroyShaderModule(device, frag_shader_module, nullptr);
        vkDestroyShaderModule(device, vert_shader_module, nullptr);
    }

    VkShaderModule TriangleApp::create_shader_module(const std::vector<char>& code) {
        VkShaderModuleCreateInfo create_info = {};
        create_info.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize                 = code.size();
        create_info.pCode                    = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shader_module;
        if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create a shader module!");
        }

        return shader_module;
    }

    /**
     * Vulkan needs to be told about the frame buffer attachments that will be used while renderering. This includes how
     * many colour and depth buffers there will be, samples per buffer, and how their consents should be used throughout
     * the renderering operations.
     */
    void TriangleApp::create_render_pass() {

        /**
         * The format of the colour attachments just need to the match the format of the swap chain images.
         */
        VkAttachmentDescription color_attachment = {};
        color_attachment.format                  = swap_chain_image_format;
        color_attachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
        
        /**
         * The load/store ops determine what we do wit hthe data in the attachment before and after rendering.
         */
        color_attachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        /**
         * Images need to be transition to specific layouts that are suitable for the operation that they're going to be
         * involved in next.
         */
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // TODO: Reread and understand the subpass directives.
        VkAttachmentReference color_attachment_ref = {};
        color_attachment_ref.attachment            = 0;
        color_attachment_ref.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments    = &color_attachment_ref;

        /**
         * Subpass dependencies specifiy the mem and execution dependncies between the subpasses. 
         */
        VkSubpassDependency dependency = {};
        dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass          = 0;
        dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask       = 0;
        dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo render_pass_info = {};
        render_pass_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount        = 1;
        render_pass_info.pAttachments           = &color_attachment;
        render_pass_info.subpassCount           = 1;
        render_pass_info.pSubpasses             = &subpass;
        render_pass_info.dependencyCount        = 1;
        render_pass_info.pDependencies          = &dependency;

        if (vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create render pass!");
        }
    }

    void TriangleApp::create_frame_buffers() {
        swap_chain_frame_buffers.resize(swap_chain_image_views.size());
        for (size_t i = 0; i < swap_chain_image_views.size(); i++) {
            VkImageView attachments[] = {
                swap_chain_image_views[i]
            };

            VkFramebufferCreateInfo frame_buffer_info = {};
            frame_buffer_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            frame_buffer_info.renderPass              = render_pass;
            frame_buffer_info.attachmentCount         = 1;
            frame_buffer_info.pAttachments            = attachments;
            frame_buffer_info.width                   = swap_chain_extent.width;
            frame_buffer_info.height                  = swap_chain_extent.height;
            frame_buffer_info.layers                  = 1;

            if (vkCreateFramebuffer(device, &frame_buffer_info, nullptr, &swap_chain_frame_buffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create frame buffer!");
            }
        }
    }

    void TriangleApp::create_command_pool() {
        QueueFamilyIndices queue_family_indices = find_queue_families(this->physical_device);

        /**
         * Cmd buffers are executed by submitting it to a device queue and can only allocate cmd buffers that are submitted to a single
         * type of queue. 
         * Two possible command flags for the pool:
         * VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: indicates that the buffers are rerecorded often
         * VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allows cmd buffers to be rerecorded individually
         */
        VkCommandPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();

        if (vkCreateCommandPool(device, &pool_info, nullptr, &command_pool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create the cmd pool!");
        }
    }

    void TriangleApp::create_command_buffers() {
        command_buffers.resize(swap_chain_frame_buffers.size());

        /*
         * The level param specifies if the buffer is a primary or secondary buffer.
         * Primary buffers can be submitted to a queue for execution, but cannot be called from other cmd buffers
         * Secondary buffers can't be submitted directly but can be called from primary buffers (you can reuse operations on the primary cmd buffers)
         */
        VkCommandBufferAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = command_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = (uint32_t)command_buffers.size();

        if (vkAllocateCommandBuffers(device, &alloc_info, command_buffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate cmd buffers!");
        }

        for (size_t i = 0; i < command_buffers.size(); i++) {
            /*
             * Flags determine how the cmd buffer is going to be used
             * VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : the cmd buffer will be rerecorded right after executing it once
             * VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : This is a secondary cd buffer that will be entirely within a render pass
             * VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: The cmd buffer can be resubmitted while also already pending execution
             */
            VkCommandBufferBeginInfo begin_info = {};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            begin_info.pInheritanceInfo = nullptr;

            if (vkBeginCommandBuffer(command_buffers[i], &begin_info) != VK_SUCCESS) {
                throw std::runtime_error("Failed to begin recording cmd buffer!");
            }

            VkRenderPassBeginInfo render_pass_info = {};
            render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            render_pass_info.renderPass = render_pass;
            render_pass_info.framebuffer = swap_chain_frame_buffers[i];

            /*
             * Render area defines where the shaders get loaded and stored. Any pixels outside the region has undefined vals
             */
            render_pass_info.renderArea.offset = {0, 0};
            render_pass_info.renderArea.extent = swap_chain_extent;

            VkClearValue clear_colour = { 0.0f, 0.0f, 0.0f, 1.0f };
            render_pass_info.clearValueCount = 1;
            render_pass_info.pClearValues = &clear_colour;

            /*
             * VK_SUBPASS_CONTENTS_INLINE: The render pass cmds will be embedded in the primary cmd buffer itself, no secondary cmds
             * will be executed
             * VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS : The render pass cmds will be executed from the 2ndary buffers
             */
            vkCmdBeginRenderPass(command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

            VkBuffer vertex_buffers[] = { vertex_buffer };
            VkDeviceSize offsets[]    = { 0 };
            vkCmdBindVertexBuffers(command_buffers[i], 0, 1, vertex_buffers, offsets);

            vkCmdDraw(command_buffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);
            vkCmdEndRenderPass(command_buffers[i]);

            if (vkEndCommandBuffer(command_buffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to record the command buffer!");
            }
        }
    }

    /*
     * Draw frame will grab an available img from the swap chain, execute the cmd buffer with the img, return the img to the swap chain 
     * for presentation.
     */
    void TriangleApp::draw_frame() {
        vkWaitForFences(device, 1, &flight_fences[current_frame], VK_TRUE, std::numeric_limits<uint64_t>::max());

        uint32_t img_index;
        VkResult result = vkAcquireNextImageKHR(device, swap_chain, std::numeric_limits<uint64_t>::max(), 
            img_available_semaphores[current_frame], VK_NULL_HANDLE, &img_index);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreate_swap_chain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("Failed to acquire swap chain img!");
        }

        VkSubmitInfo submit_info = {};
        submit_info.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore wait_semaphores[]      = { img_available_semaphores[current_frame] };
        VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submit_info.waitSemaphoreCount     = 1;
        submit_info.pWaitSemaphores        = wait_semaphores;
        submit_info.pWaitDstStageMask      = wait_stages;

        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers    = &command_buffers[img_index];

        VkSemaphore signal_semaphores[]  = { render_finished_semaphores[current_frame] };
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores    = signal_semaphores;

        vkResetFences(device, 1, &flight_fences[current_frame]);

        if (vkQueueSubmit(graphics_queue, 1, &submit_info, flight_fences[current_frame]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit draw cmd buffer!");
        }

        VkPresentInfoKHR present_info   = {};
        present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores    = signal_semaphores;

        VkSwapchainKHR swap_chains[] = { swap_chain };
        present_info.swapchainCount  = 1;
        present_info.pSwapchains     = swap_chains;
        present_info.pImageIndices   = &img_index;

        result = vkQueuePresentKHR(present_queue, &present_info);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || frame_buffer_resized_flag) {
            frame_buffer_resized_flag = true;
            recreate_swap_chain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swap chain img!");
        }

        current_frame = (current_frame + 1) % max_frames_per_flight;
    }

    void TriangleApp::create_sync_objects() {
        img_available_semaphores.resize(max_frames_per_flight);
        render_finished_semaphores.resize(max_frames_per_flight);
        flight_fences.resize(max_frames_per_flight);

        VkSemaphoreCreateInfo semaphore_info = {};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fence_info = {};
        /**
         * Fences are created in the unsignaled state so nothing will render until the fence has been used before.
         * To solve this, set the fence to be created in the signaled state.
         */
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (int i = 0; i < max_frames_per_flight; i++) {
            if (vkCreateSemaphore(device, &semaphore_info, nullptr, &img_available_semaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphore_info, nullptr, &render_finished_semaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fence_info, nullptr, &flight_fences[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create signals!");
            }
        }
    }

    void TriangleApp::recreate_swap_chain() {
        int width = 0, height = 0;

        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(this->device);

        create_swap_chain();
        create_image_views();
        create_render_pass();
        create_graphics_pipeline();
        create_frame_buffers();
        create_frame_buffers();
    }

    void TriangleApp::create_vertex_buffer() {
        VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

        /**
         * The staging buffer and staging buffer memory allows us to fast copy the original vertbux buffer in temporary 
         * buffer (double buffering technique).
         *
         * VK_BUFFER_USAGE_TRANSFER_SRC_BIT: buffer can be used as a source for a copy.
         * VK_BUFFER_USAGE_TRANSFER_DST_BIT: buffer can be used as a the pointer to where the copy will go to.
         */
        VkBuffer staging_buffer;
        VkDeviceMemory staging_buffer_memory;
        create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

        /**
         * Store the memory and map it to the data pointer.
         *
         * The mapped memory must copy to the buffer so we ensure that at any point in time the memory is the same. So
         * this can lead to performance problems since we don't flush the memory immediately.
         */
        void* data;
        vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
        memcpy(data, vertices.data(), (size_t)buffer_size);
        vkUnmapMemory(device, staging_buffer_memory);

        // TODO: Rework the vertex buffer creation to use a staging buffer technique.
        create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer, vertex_buffer_memory);

        copy_buffer(staging_buffer, vertex_buffer, buffer_size);
        vkDestroyBuffer(device, staging_buffer, nullptr);
        vkFreeMemory(device, staging_buffer_memory, nullptr);
    }

    /**
     * We iterate throughout the memory and determine what kind of memory the space is. If the filter and the bit flag
     * is 1 and we can allow certain properties within the mem type, then we can return the index.
     */
    uint32_t TriangleApp::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags props) {
        VkPhysicalDeviceMemoryProperties mem_props;
        vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);

        for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
            if (type_filter & (1 << i) && (mem_props.memoryTypes[i].propertyFlags & props) == props) {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable mem types!");
    }

    /**
     * More generic function so we can create tons of buffers.
     */
    void TriangleApp::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, 
        VkBuffer& buffer, VkDeviceMemory& buffer_mem) {

        VkBufferCreateInfo buffer_info = {};
        buffer_info.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size               = size;
        buffer_info.usage              = usage;
        buffer_info.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &buffer_info, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create vertex buffer!");
        }

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(device, buffer, &mem_requirements);

        /**
         * Specify the size and the type which derives from the memory requirements of the vertex buffer.
         */
        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize       = mem_requirements.size;
        alloc_info.memoryTypeIndex      = find_memory_type(mem_requirements.memoryTypeBits, props);

        if (vkAllocateMemory(device, &alloc_info, nullptr, &buffer_mem) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate vertex buffer memory!");
        }

        vkBindBufferMemory(device, buffer, buffer_mem, 0);
    }

    /**
     * To do memory based operations, we need to use a cmd_buffer. The buffers are temporary and we can likely handle
     * this elsewhere. If we create a generic cmd buffer for memory cache optimizations, then we should use a 
     * VK_COMMAND_POOL_CREATE_TRANSIENT_BIT flag.
     */
    void TriangleApp::copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
        VkCommandBufferAllocateInfo alloc_info = {};
        alloc_info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandPool                 = this->command_pool;
        alloc_info.commandBufferCount          = 1;

        VkCommandBuffer cmd_buffer;
        vkAllocateCommandBuffers(device, &alloc_info, &cmd_buffer);

        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
       
        // Start recording the cmd buffer
        vkBeginCommandBuffer(cmd_buffer, &begin_info);
       
        // Do the actual cmd buffer copy!
        VkBufferCopy copy_region = {};
        copy_region.size         = size;

        vkCmdCopyBuffer(cmd_buffer, src, dst, 1, &copy_region);
        vkEndCommandBuffer(cmd_buffer);

        VkSubmitInfo submit_info       = {};
        submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers    = &cmd_buffer;

        // Execute the transfer immediately by using a wait for idle, I could use a fence here and allow waiting 
        // for the cmd buffer to finish executing too.
        vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphics_queue);

        vkFreeCommandBuffers(device, command_pool, 1, &cmd_buffer);
    }
}
