#define GLFW_INCLUDE_VULKAN

#include "TriangleApp.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <set>
#include <vulkan/vulkan.h>

namespace vulkan_rendering {

    void TriangleApp::run() {
        init_window();
        init_vulkan();
        main_loop();
        cleanup();
    }

    void TriangleApp::init_window() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // Disable the window hint.
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void TriangleApp::init_vulkan() {
        create_instance();
        setup_debugger();
        create_surface();
        auto validation = [this](VkPhysicalDevice device) -> bool {
            QueueFamilyDevice indices = queue_families(device);
            bool is_extension_supported = check_device_extension_support(device);

            bool is_swap_chain_valid = false;
            if (is_extension_supported) {
                auto swap_chain_support = query_swap_chain_support(device);
                is_swap_chain_valid = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
            }

            return indices.is_complete() && is_extension_supported && is_swap_chain_valid;
        };
        select_physical_device(validation);
        create_logical_device();
    }

    void TriangleApp::main_loop() {
        while (!glfwWindowShouldClose(window)) {
            // Loop and wait until we check for an event.
            glfwPollEvents();
        }
    }

    void TriangleApp::cleanup() {
        if (enable_validation_layers) {
            Destroy_Debug_Utils_Messenger(instance, debug_messenger, nullptr);
        }
        vkDestroyInstance(instance, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
        vkDestroyDevice(device, nullptr);
    }

    void inline TriangleApp::create_instance() {
        if (enable_validation_layers && !check_validation_support()) {
            throw std::runtime_error("Validation layers requested but not available!");
        }

        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "Triangle App";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "No Engine";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;

        uint32_t glfw_extension_count = 0;
        const char** glfw_extension;

        glfw_extension = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        create_info.enabledExtensionCount = glfw_extension_count;
        create_info.ppEnabledExtensionNames = glfw_extension;
        create_info.enabledLayerCount = 0;

        auto extensions = get_required_extensions();
        create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();

        if (enable_validation_layers) {
            create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
            create_info.ppEnabledLayerNames = validation_layers.data();
        }
        else {
            create_info.enabledLayerCount = 0;
        }

        if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create VkInstance!");
        }
    }

    inline void TriangleApp::create_logical_device() {
        QueueFamilyDevice indices = queue_families(physical_device);

        std::vector<VkDeviceQueueCreateInfo> queue_create_info;
        std::set<uint32_t> unique_queue_families = { indices.graphics_family.value(), indices.present_family.value() };

        float queuePriority = 1.0f;
        for (uint32_t queue_family : unique_queue_families) {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queue_family;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queue_create_info.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures device_features = {};

        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_info.size());
        create_info.pQueueCreateInfos = queue_create_info.data();
        create_info.pEnabledFeatures = &device_features;
        create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
        create_info.ppEnabledExtensionNames = device_extensions.data();

        if (enable_validation_layers) {
            create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
            create_info.ppEnabledLayerNames = validation_layers.data();
        }
        else {
            create_info.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create logical device!");
        }

        vkGetDeviceQueue(device, indices.graphics_family.value(), 0, &graphics_queue);
        vkGetDeviceQueue(device, indices.present_family.value(), 0, &present_queue);
    }

    inline void TriangleApp::create_surface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    inline bool TriangleApp::check_validation_support() {
        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        for (const auto layer_name : validation_layers) {
            bool is_found = false;

            for (const auto layer_properties : available_layers) {
                if (strcmp(layer_name, layer_properties.layerName) == 0) {
                    is_found = true;
                    break;
                }
            }

            if (!is_found) {
                return false;
            }
        }

        return true;
    }

    inline bool TriangleApp::check_device_extension_support(VkPhysicalDevice device) {
        uint32_t extension_size;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_size, nullptr);

        std::vector<VkExtensionProperties> available_extensions(extension_size);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_size, available_extensions.data());

        std::set<std::string> required(device_extensions.begin(), device_extensions.end());

        for (const auto& extension : available_extensions) {
            required.erase(extension.extensionName);
        }

        return required.empty();
    }

    std::vector<const char*> TriangleApp::get_required_extensions() {
        uint32_t extension_count = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extension_count);
        std::vector<const char*> extensions(glfw_extensions, glfw_extensions + extension_count);

        if (enable_validation_layers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    QueueFamilyDevice TriangleApp::queue_families(VkPhysicalDevice device) {
        QueueFamilyDevice indices;
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

    VKAPI_ATTR VkBool32 VKAPI_CALL TriangleApp::debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        // We can make message severities show if they hit pass a certain threshold.
        // In this case we want to show all messages which are warnings and errors, here are some examples below...
        // VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: Some event has happened that is unrelated to the specification or performance
        // VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: Something has happened that violates the specification or indicates a possible mistake
        // VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: Potential non-optimal use of Vulkan
        if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            std::cerr << "validation layers" << pCallbackData->pMessage << std::endl;
        }

        return VK_FALSE;
    }

    void TriangleApp::setup_debugger() {
        if (!enable_validation_layers) {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        create_info.pfnUserCallback = debug_callback;
        create_info.pUserData = nullptr; // Optional

        if (Create_Debug_Utils_Messenger(instance, &create_info, nullptr, &debug_messenger)) {
            throw new std::runtime_error("Failed to set up the debugger!");
        }
    }

    void TriangleApp::select_physical_device(std::function<bool(VkPhysicalDevice)> validation) {
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
        if (device_count == 0) {
            throw std::runtime_error("Failed to find a GPU that supports Vulkan!");
        }

        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

        for (const auto& d : devices) {
            // TODO: Add a conditional check, possibly pass that as a lambda expression
            if (validation(d)) {
                physical_device = d;
                break;
            }
        }

        if (device == VK_NULL_HANDLE) {
            throw std::runtime_error("Failed to find a suitable GPU");
        }
    }
}
