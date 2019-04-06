#ifndef TRIANGLE_APP_H
#define TRIANGLE_APP_H
#define GLFW_INCLUDE_VULKAN

#include "QueueFamilyDevice.h"
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
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
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

		VkPhysicalDevice physical_device;
		VkDevice device;
		VkQueue graphics_queue;
		VkQueue present_queue;
		VkSurfaceKHR surface;

#if NDEBUG
		const bool enable_validation_layers = false;
#else
		const bool enable_validation_layers = true;
#endif

		GLFWwindow* window;
		VkInstance instance;
		VkDebugUtilsMessengerEXT debug_messenger;

		void init_window();
		void init_vulkan();
		void main_loop();
		void cleanup();
		void setup_debugger();
		void select_physical_device(std::function<bool(VkPhysicalDevice)> validation);
		void inline create_instance();
		void inline create_logical_device();
		void inline create_surface();
		bool inline check_validation_support();
		std::vector<const char*> get_required_extensions();
		QueueFamilyDevice queue_families(VkPhysicalDevice device);

		static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);
	};
}

#endif