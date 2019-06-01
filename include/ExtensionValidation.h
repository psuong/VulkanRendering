#ifndef EXTENSION_VALIDATION_H
#define EXTENSION_VALIDATION_H
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <set>
#include <vector>
#include <vulkan/vulkan.h>

namespace vulkan_rendering {

    class ExtensionValidation {

        private:
            std::set<const char*> extensions_cache;

        public:
            ExtensionValidation();
            void populate(std::vector<VkExtensionProperties> extensions);
            bool validate_glfw_extensions(const char** glfw_extensions);
    };
}
#endif
