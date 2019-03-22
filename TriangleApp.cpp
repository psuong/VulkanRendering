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
}