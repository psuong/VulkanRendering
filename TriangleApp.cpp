#include "TriangleApp.h"

#define GLFW_INCLUDE_VULKAN
#include <iostream>
#include <GLFW/glfw3.h>
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
	}

	void TriangleApp::main_loop() {
		while (!glfwWindowShouldClose(window)) {
			// Loop and wait until we check for an event.
			glfwPollEvents();
		}
	}

	void TriangleApp::cleanup() {
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void inline TriangleApp::create_instance() {
		if (enable_validation_layers && !check_validation_support()) {
			throw std::runtime_error("Validation layers requested but not available!");
		}

		VkApplicationInfo app_info  = {};
		app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName   = "Triangle App";
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName        = "No Engine";
		app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
		app_info.apiVersion         = VK_API_VERSION_1_0;

		VkInstanceCreateInfo create_info = {};
		create_info.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo     = &app_info;

		uint32_t glfw_extension_count = 0;
		const char** glfw_extension;

		glfw_extension                      = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
		create_info.enabledExtensionCount   = glfw_extension_count;
		create_info.ppEnabledExtensionNames = glfw_extension;
		create_info.enabledLayerCount       = 0;

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

	bool inline TriangleApp::check_validation_support() {
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

	std::vector<const char*> TriangleApp::get_required_extensions() {
		uint32_t extension_count = 0;
		const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extension_count);
		std::vector<const char*> extensions(glfw_extensions, glfw_extensions + extension_count);

		if (enable_validation_layers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL TriangleApp::debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
		VkDebugUtilsMessageTypeFlagsEXT messageType, 
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
		void* pUserData) {

		std::cerr << "validation layers" << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}
}