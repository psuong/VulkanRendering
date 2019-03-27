#ifndef TRIANGLE_APP_H
#define TRIANGLE_APP_H

#include <GLFW/glfw3.h>
#include <vector>
#include <vulkan/vulkan.h>

namespace vulkan_rendering {
	
	class TriangleApp {
		
	public:
		void run();

	private:
		const int WIDTH = 800;
		const int HEIGHT = 600;
		const std::vector<const char*> validation_layers = {
			"VK_LAYER_LUNARG_standard_validation"
		};

#if NDEBUG
		const bool enable_validation_layers = false;
#else
		const bool enable_validation_layers = true;
#endif

		GLFWwindow* window;
		VkInstance instance;

		void init_window();
		void init_vulkan();
		void main_loop();
		void cleanup();
		void inline create_instance();
		bool inline check_validation_support();
		std::vector<const char*> get_required_extensions();
	};
}

#endif