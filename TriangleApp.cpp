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
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void TriangleApp::create_instance() {
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

		if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create VkInstance!");
		}
	}
}