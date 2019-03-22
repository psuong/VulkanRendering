#ifndef TRIANGLE_APP_H
#define TRIANGLE_APP_H

#include <GLFW/glfw3.h>

namespace vulkan_rendering {
	
	class TriangleApp {
		
	public:
		void run();

	private:
		const int WIDTH = 800;
		const int HEIGHT = 600;
		GLFWwindow* window;

		void init_window();
		void init_vulkan();
		void main_loop();
		void cleanup();
	};
}

#endif