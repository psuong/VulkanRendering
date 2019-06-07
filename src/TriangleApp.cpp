#define GLFW_INCLUDE_VULKAN

#include "../include/ExtensionValidation.h"
#include "../include/QueueFamilyIndices.h"
#include "../include/SwapChainSupportDetails.h"
#include "../include/TriangleApp.h"

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
    }

    void TriangleApp::init_vulkan() {
        create_instance();
        setup_debug_messenger();
        create_surface();
        pick_physical_device();
        create_logical_device();
        create_swap_chain();
    }

    void TriangleApp::main_loop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void TriangleApp::cleanup() {
        vkDestroySwapchainKHR(device, swap_chain, nullptr);
        vkDestroyDevice(device, nullptr);

        if (enable_validation_layers) {
            destroy_debug_utils_messenger_ext(instance, debug_messenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
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
            queue_create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
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
            VkExtent2D actual_extent = { (uint32_t)WIDTH, (uint32_t)HEIGHT };

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
}
