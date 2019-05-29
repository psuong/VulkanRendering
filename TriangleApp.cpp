#define GLFW_INCLUDE_VULKAN

#include "FileHelper.h"
#include "TriangleApp.h"
#include <algorithm>
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

            bool extensionsSupported = check_device_extension_support(device);

            bool swapChainAdequate = false;
            if (extensionsSupported) {
                SwapChainSupportDetails swapChainSupport = query_swap_chain_support(device);
                swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.present_modes.empty();
            }

            return indices.is_complete() && extensionsSupported&& swapChainAdequate;
        };
        select_physical_device(validation);
        create_logical_device();
        create_swap_chain();
        create_image_views();

        // Before we create the pipelinem we need the render pass which defines a lot of the info for the pipeline.
        create_render_pass();
        create_graphics_pipeline();

        create_frame_buffers();
        create_command_pool();
        create_command_buffers();
    }

    void TriangleApp::main_loop() {
        while (!glfwWindowShouldClose(window)) {
            // Loop and wait until we check for an event.
            glfwPollEvents();
            draw_frame();
        }
    }

    void TriangleApp::cleanup() {
        vkDestroySemaphore(device, render_finished_semaphore, nullptr);
        vkDestroySemaphore(device, image_available_semaphore, nullptr);

        vkDestroyCommandPool(device, command_pool, nullptr);
        for (auto frameBuffer : swapchain_frame_buffers) {
            vkDestroyFramebuffer(device, frameBuffer, nullptr);
        }

        vkDestroyPipeline(device, graphics_pipeline, nullptr);
        vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
        vkDestroyRenderPass(device, render_pass, nullptr);

        for (auto image : swapchain_image_views) {
            vkDestroyImageView(device, image, nullptr);
        }
        vkDestroySwapchainKHR(device, swap_chain, nullptr);
        vkDestroyDevice(device, nullptr);

        if (enable_validation_layers) {
            Destroy_Debug_Utils_Messenger(instance, debug_messenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
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

    VkSurfaceFormatKHR TriangleApp::select_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats) {
        if (available_formats.size() == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED) {
            // NOTE: We want to work in SRGB colour space but we use the standard RGB colours to work in since it's a tad easier.
            return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
        }

        for (const auto& available_format : available_formats) {
            if (available_format.format == VK_FORMAT_B8G8R8A8_UNORM &&
                available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return available_format;
            }
        }

        // Default return value for the swap surface support, you can usually just pick your own.
        // https://stackoverflow.com/questions/12524623/what-are-the-practical-differences-when-working-with-colours-in-a-linear-vs-a-no
        return available_formats[0];
    }

    VkPresentModeKHR TriangleApp::select_presentation_mode(const std::vector<VkPresentModeKHR>& available_presentations) {
        auto best_option = VK_PRESENT_MODE_FIFO_KHR;
        for (const auto& presentation : available_presentations) {
            // We want to support triple buffering first but if it's not available we choose the next best.
            if (presentation == VK_PRESENT_MODE_MAILBOX_KHR) {
                return presentation;
            }
            else if (presentation == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                best_option = presentation;
            }
        }

        return best_option;
    }

    VkExtent2D TriangleApp::select_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        else {
            VkExtent2D actual = { WIDTH, HEIGHT };

            actual.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actual.width));
            actual.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actual.height));

            return actual;
        }
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
            bool is_valid = validation(d);
            if (is_valid) {
                physical_device = d;
                break;
            }
        }

        if (physical_device == VK_NULL_HANDLE) {
            throw std::runtime_error("Failed to find a suitable GPU");
        }
    }

    void TriangleApp::create_swap_chain() {
        auto swap_chain_support = query_swap_chain_support(physical_device);

        VkSurfaceFormatKHR surface_format = select_swap_surface_format(swap_chain_support.formats);
        VkPresentModeKHR present_mode = select_presentation_mode(swap_chain_support.present_modes);
        VkExtent2D extent = select_swap_extent(swap_chain_support.capabilities);

        // How many images do we want to support in the swap chain?
        uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;

        if (swap_chain_support.capabilities.minImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount) {
            image_count = swap_chain_support.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface = surface;
        create_info.minImageCount = image_count;
        create_info.imageFormat = surface_format.format;
        create_info.imageColorSpace = surface_format.colorSpace;
        create_info.imageExtent = extent;
        create_info.imageArrayLayers = 1;                                  // specifies the # of layers each image consists of, normally it's usually just 1.
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyDevice indices = queue_families(physical_device);
        uint32_t queue_family_indices[] = { indices.graphics_family.value(), indices.present_family.value() };

        if (indices.graphics_family != indices.present_family) {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = 2;
            create_info.pQueueFamilyIndices = queue_family_indices;
        }
        else {
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            create_info.queueFamilyIndexCount = 0; // Optional
            create_info.pQueueFamilyIndices = nullptr; // Optional
        }

        create_info.preTransform = swap_chain_support.capabilities.currentTransform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.presentMode = present_mode;
        create_info.clipped = VK_TRUE;
        create_info.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &create_info, nullptr, &swap_chain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        // NOTE: Why we do we do this?
        // Because we only specified a minimum number of images in the swap chain, so we want to query all potential images
        // first. Then we want to resize the containers and get all the image handles again.
        vkGetSwapchainImagesKHR(device, swap_chain, &image_count, nullptr);
        swap_chain_images.resize(image_count);
        vkGetSwapchainImagesKHR(device, swap_chain, &image_count, swap_chain_images.data());

        swap_chain_image_format = surface_format.format;
        swap_chain_extent       = extent;
    }

    void TriangleApp::create_image_views() {
        // Resize the buffer to fit all images we want to create.
        swapchain_image_views.resize(swap_chain_images.size());

        for (auto i = 0; i < swap_chain_images.size(); i++) {
            VkImageViewCreateInfo create_info = {};
            create_info.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            create_info.format                = swap_chain_image_format;
            create_info.image = swap_chain_images[i];
            
            // We can define how the image data can be interpretted, e.g. as a regular 2D image or even a cube map.
            create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            create_info.format = swap_chain_image_format;

            create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            // What is our image's purpose is described via the subresourceRange.
            // In this case, we want colour targets without any mipmapping.
            create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            create_info.subresourceRange.baseMipLevel   = 0;
            create_info.subresourceRange.levelCount     = 1;
            create_info.subresourceRange.baseArrayLayer = 0;
            create_info.subresourceRange.layerCount     = 1;

            if (vkCreateImageView(device, &create_info, nullptr, &swapchain_image_views[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create image view!");
            }
        }
    }

    void TriangleApp::create_graphics_pipeline() {
        auto vertex_shader   = Read_File("shaders/vert.spv");
        auto fragment_shader = Read_File("shaders/frag.spv");

        // Shader modules can be destroyed as soon as the pipeline creation is finished, so rather than scope bound them
        // to the class, we can provide the closure around a function instead.
        auto vertex_shader_module   = create_shader_module(vertex_shader);
        auto fragment_shader_module = create_shader_module(fragment_shader);

        VkPipelineShaderStageCreateInfo vertex_pipeline_info = {};
        vertex_pipeline_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertex_pipeline_info.stage                           = VK_SHADER_STAGE_VERTEX_BIT;
        vertex_pipeline_info.module                          = vertex_shader_module;
        vertex_pipeline_info.pName                           = "main";

        // Another neat trick is to allow pSpecializationInfo and define various kinds of constants for different stages 
        // of the pipeline workflow.

        VkPipelineShaderStageCreateInfo fragment_pipeline_info = {};
        fragment_pipeline_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragment_pipeline_info.stage                           = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragment_pipeline_info.module                          = fragment_shader_module;
        fragment_pipeline_info.pName                           = "main";

        VkPipelineShaderStageCreateInfo shader_stages[] = { vertex_pipeline_info, fragment_pipeline_info };

        // Vertex input
        VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
        vertex_input_info.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_info.vertexBindingDescriptionCount        = 0;
        vertex_input_info.vertexAttributeDescriptionCount      = 0;

        // TODO: Since I'm hard coding the bindings and attributes directly into the shader, I don't need to specify this yet
        vertex_input_info.pVertexBindingDescriptions   = nullptr;
        vertex_input_info.pVertexAttributeDescriptions = nullptr;

        // Input asm
        VkPipelineInputAssemblyStateCreateInfo input_asm       = {};
        input_asm.sType                                        = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_asm.topology                                     = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_asm.primitiveRestartEnable                       = VK_FALSE;

        // Viewport
        VkViewport view_port = {};
        view_port.x          = 0.0f;
        view_port.y          = 0.0f;
        view_port.width      = (float)swap_chain_extent.width;
        view_port.height     = (float)swap_chain_extent.height;
        view_port.minDepth   = 0.0f;
        view_port.maxDepth   = 1.0f;

        // Scissor
        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = swap_chain_extent;

        // Putting viewport + scissor together
        VkPipelineViewportStateCreateInfo viewport_state = {};
        viewport_state.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.viewportCount                     = 1;
        viewport_state.pViewports                        = &view_port;
        viewport_state.scissorCount                      = 1;
        viewport_state.pScissors                         = &scissor;

        // TODO: Integrate the rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable                       = VK_FALSE;
        rasterizer.rasterizerDiscardEnable                = VK_FALSE; // If enabled to true then nothing would be rendered.
        rasterizer.polygonMode                            = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth                              = 1.0f;
        rasterizer.depthBiasEnable                        = VK_FALSE;
        rasterizer.depthBiasConstantFactor                = 0.0f; // Optional
        rasterizer.depthBiasClamp                         = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor                   = 0.0f; // Optional

        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable                  = VK_FALSE;
        multisampling.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading                     = 1.0f; // Optional
        multisampling.pSampleMask                          = nullptr; // Optional
        multisampling.alphaToCoverageEnable                = VK_FALSE; // Optional
        multisampling.alphaToOneEnable                     = VK_FALSE; // Optional
        
        // TODO: Add support for depth and stencil testing

        // Colour blending
        // How does it work?
        // If we enable blending, we perform the following operations: 
        // final_colour.rgb = (source_colour_blending * new_colour.rgb) <some-operation> (destination_colour_blend_factor * old_colour).rbg
        // final_color.a = (source_alpha_blend * new_color.a) <some-operation> (destination_alpha_blend * old_color.a)
        // If we disable blending, we just take the new colour and overwrite the previous colour
        // Most likely, we would want to perform some sort of colour lerping based on the alpha
        // final_colour.rgb = new_alpha * new_colour + (1 - newAlpha) * old_colour;
        // final_colour.a = new_alpha.a;
        // To use the second option to allow bitwise operations set the logicOpEnabled to true
        VkPipelineColorBlendAttachmentState colour_blend_attachment = {};
        colour_blend_attachment.colorWriteMask                      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colour_blend_attachment.blendEnable                         = VK_TRUE;
        colour_blend_attachment.srcColorBlendFactor                 = VK_BLEND_FACTOR_SRC_ALPHA;
        colour_blend_attachment.dstColorBlendFactor                 = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colour_blend_attachment.colorBlendOp                        = VK_BLEND_OP_ADD;
        colour_blend_attachment.srcAlphaBlendFactor                 = VK_BLEND_FACTOR_ONE;
        colour_blend_attachment.dstAlphaBlendFactor                 = VK_BLEND_FACTOR_ZERO;
        colour_blend_attachment.alphaBlendOp                        = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colour_blending = {};
        colour_blending.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colour_blending.logicOpEnable                       = VK_FALSE;
        colour_blending.logicOp                             = VK_LOGIC_OP_COPY; // Optional
        colour_blending.attachmentCount                     = 1;
        colour_blending.pAttachments                        = &colour_blend_attachment;
        colour_blending.blendConstants[0]                   = 0.0f; // Optional
        colour_blending.blendConstants[1]                   = 0.0f; // Optional
        colour_blending.blendConstants[2]                   = 0.0f; // Optional
        colour_blending.blendConstants[3]                   = 0.0f; // Optional

        // TODO: Fill out hte pipeline later
        VkPipelineLayoutCreateInfo pipeline_layout_info = {};
        pipeline_layout_info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount             = 0; // Optional
        pipeline_layout_info.pSetLayouts                = nullptr; // Optional
        pipeline_layout_info.pushConstantRangeCount     = 0; // Optional
        pipeline_layout_info.pPushConstantRanges        = nullptr; // Optional

        if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipeline_info = {};
        pipeline_info.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount                   = 2;
        pipeline_info.pStages                      = shader_stages;
        pipeline_info.pVertexInputState            = &vertex_input_info;
        pipeline_info.pInputAssemblyState          = &input_asm;
        pipeline_info.pViewportState               = &viewport_state;
        pipeline_info.pRasterizationState          = &rasterizer;
        pipeline_info.pMultisampleState            = &multisampling;
        pipeline_info.pColorBlendState             = &colour_blending;
        pipeline_info.layout                       = pipeline_layout;
        pipeline_info.renderPass                   = render_pass;
        pipeline_info.subpass                      = 0;

        // So we can provide a secondary pipeline that is similar to the current pipeline and an index so that we can 
        // switch between them. It is easier and less expensive to switch between two pipelines that are similar.
        pipeline_info.basePipelineHandle           = VK_NULL_HANDLE;
        pipeline_info.basePipelineIndex = -1;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create graphics pipeline!");
        }
        vkDestroyShaderModule(device, vertex_shader_module, nullptr);
        vkDestroyShaderModule(device, fragment_shader_module, nullptr);
    }

    VkShaderModule TriangleApp::create_shader_module(const std::vector<char>& code) {
        VkShaderModuleCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = code.size();

        // NOTE: Reinterpret the pointer.
        create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shader_module;
        if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
            throw new std::runtime_error("Failed to create the shader module!");
        }

        return shader_module;
    }


    // Need to specify how many colours and depth buffers there will be for rendering.
    void TriangleApp::create_render_pass() {

        VkAttachmentDescription colour_attachment = {};
        colour_attachment.format                  = swap_chain_image_format;

        // Sticking to 1 sample because I'm not doing anything special for this RP.
        colour_attachment.samples = VK_SAMPLE_COUNT_1_BIT;

        // Load operations: what do we do with the data before rendering?
        /*
         * VK_ATTACHMENT_LOAD_OP_LOAD: Preserve the existing contents of the attachment
         * VK_ATTACHMENT_LOAD_OP_CLEAR: Clear the values to a constant at the start
         * VK_ATTACHMENT_LOAD_OP_DONT_CARE: Existing contents are undefined; we don't care about them 
         */
        colour_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

        // Store operations, what do we do with the data after rendering?
        /*
         * VK_ATTACHMENT_STORE_OP_STORE: Rendered contents will be stored in memory and can be read later
         * VK_ATTACHMENT_STORE_OP_DONT_CARE: Contents of the framebuffer will be undefined after the rendering operation 
         */
        // 
        colour_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        // Not doing anything with the stencil operations.
        // Stencil buffer example: https://computergraphics.stackexchange.com/questions/12/what-is-a-stencil-buffer
        colour_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colour_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        // Layouts determine how the pixels are laid out in memory
        /* Some of the common layouts in Vulkan
         * VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Images used as color attachment
         * VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: Images to be presented in the swap chain
         * VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: Images to be used as destination for a memory copy operation
        */
        colour_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colour_attachment.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colour_attach_ref = {};
        colour_attach_ref.attachment            = 0; // Reference which description to use in the attachment array.
        colour_attach_ref.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // We want to display colour in an optimal way.

        // The index of the attachment in the array is reference to the layout(location = 0) out vec4 outColour;
        /* Some other attachments we can use during a subpass.
         * pInputAttachments      : Attachments that are read from a shader
         * pResolveAttachments    : Attachments used for multisampling color attachments
         * pDepthStencilAttachment: Attachment for depth and stencil data
         * pPreserveAttachments   : Attachments that are not used by this subpass, but for which the data must be preserved 
         */
        VkSubpassDescription subpass_desc = {};
        subpass_desc.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_desc.colorAttachmentCount = 1;
        subpass_desc.pColorAttachments    = &colour_attach_ref;
        // Subpasses automatically take care of image layout transitions. The transitions are controlled by the subpass
        // dependencies. This specifies the memory and execution dependencies.
        VkSubpassDependency dependency    = {};
        dependency.srcSubpass             = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass             = 0;

        // We want to specify the options to wait for and the stages in which these operations occur.
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;

        // The operations should wait on this are in the color attachment stage and involve reading/writing of colour 
        // attachment.
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


        VkRenderPassCreateInfo render_pass_info = {};
        render_pass_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount        = 1;
        render_pass_info.pAttachments           = &colour_attachment;
        render_pass_info.subpassCount           = 1;
        render_pass_info.pSubpasses             = &subpass_desc;
        render_pass_info.dependencyCount        = 1;
        render_pass_info.pDependencies          = &dependency;

        if (vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create render pass!");
        }
    }

    void TriangleApp::create_frame_buffers() {
        swapchain_frame_buffers.resize(swapchain_image_views.size());

        for (size_t i = 0; i < swapchain_image_views.size(); i++) {
            VkImageView attachments[] = {
                swapchain_image_views[i]
            };

            VkFramebufferCreateInfo framebuffer_info = {};
            framebuffer_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_info.renderPass              = render_pass;
            framebuffer_info.attachmentCount         = 1;
            framebuffer_info.pAttachments            = attachments;
            framebuffer_info.width                   = swap_chain_extent.width;
            framebuffer_info.height                  = swap_chain_extent.height;
            framebuffer_info.layers                  = 1;

            if (vkCreateFramebuffer(device, &framebuffer_info, nullptr, &swapchain_frame_buffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create the frame buffer!");
            }
        }
    }

    void TriangleApp::create_command_pool() {
        QueueFamilyDevice queue_family_devices = queue_families(physical_device);

        VkCommandPoolCreateInfo pool_info = {};
        pool_info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.queueFamilyIndex        = queue_family_devices.graphics_family.value();

        if (vkCreateCommandPool(device, &pool_info, nullptr, &command_pool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create the command pool!");
        }
    }

    void TriangleApp::create_command_buffers() {
        command_buffers.resize(swapchain_frame_buffers.size());

        VkCommandBufferAllocateInfo alloc_info = {};
        alloc_info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool                 = command_pool;
        alloc_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount          = (uint32_t)command_buffers.size();

        if (vkAllocateCommandBuffers(device, &alloc_info, command_buffers.data()) != VK_SUCCESS) {
            throw new std::runtime_error("Failed to allocate command buffers");
        }

        for (size_t i = 0; i < command_buffers.size(); i++) {
            VkCommandBufferBeginInfo begin_info = {};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

            if (vkBeginCommandBuffer(command_buffers[i], &begin_info) != VK_SUCCESS) {
                throw new std::runtime_error("Failed to begin recording the command buffer!");
            }

            VkRenderPassBeginInfo render_pass_info = {};
            render_pass_info.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            render_pass_info.renderPass            = render_pass;
            render_pass_info.framebuffer           = swapchain_frame_buffers[i];
            render_pass_info.renderArea.offset     = { 0, 0 };
            render_pass_info.renderArea.extent     = swap_chain_extent;

            VkClearValue clear_color         = { 0.0f, 0.0f, 0.0f, 1.0f };
            render_pass_info.clearValueCount = 1;
            render_pass_info.pClearValues    = &clear_color;

            // First param is the command buffer
            // Second param specifies the details of the render pass
            // VK_SUBPASS_CONTENTS_INLINE: The render pass commands will be embedded in the primary command buffer 
            // itself and no secondary command buffers will be executed.
            // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: The render pass commands will be executed from 
            // secondary command buffers.
            vkCmdBeginRenderPass(command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
            vkCmdDraw(command_buffers[i], 3, 1, 0, 0);

            // End the render pass
            vkCmdEndRenderPass(command_buffers[i]);

            if (vkEndCommandBuffer(command_buffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to record command buffer!");
            }
        }
    }

    void TriangleApp::draw_frame() {
        // Acquire an image from the swap chain
        // Execute the command buffer with that image as an attachment to the frame_buffer
        // Return the image to the swap chain for presentation

        uint32_t image_index;
        vkAcquireNextImageKHR(device, swap_chain, std::numeric_limits<uint64_t>::max(), image_available_semaphore, 
            VK_NULL_HANDLE, &image_index);

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // We want to wait during different stages that we can specify in the pipeline.
        // Our wait signal is that our image has to available before we can render it - makes sense
        VkSemaphore wait_semaphores[]      = { image_available_semaphore };
        VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submit_info.waitSemaphoreCount     = 1;
        submit_info.pWaitSemaphores        = wait_semaphores;
        submit_info.pWaitDstStageMask      = wait_stages;

        // We want to submit the command buffers which binds the swap_chain image we acquired during the colour attachment.
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers    = &command_buffers[image_index];

        VkSemaphore signal_semaphores[]  = { render_finished_semaphore };
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores    = signal_semaphores;

        if (vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit draw command buffer!");
        }

        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores    = signal_semaphores;

        VkSwapchainKHR swap_chains[] = { swap_chain };
        present_info.swapchainCount  = 1;
        present_info.pSwapchains     = swap_chains;
        present_info.pImageIndices   = &image_index;
        present_info.pResults        = nullptr;

        vkQueuePresentKHR(present_queue, &present_info);
    }

    void TriangleApp::create_semaphores() {
        VkSemaphoreCreateInfo semaphore_info = {};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        if (vkCreateSemaphore(device, &semaphore_info, nullptr, &image_available_semaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphore_info, nullptr, &render_finished_semaphore) != VK_SUCCESS) {

            throw std::runtime_error("Failed to create semaphores");
        }
    }
}
