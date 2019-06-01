#include "../include/ExtensionValidation.h"

#include <iostream>
#include <iterator>
#include <set>

namespace vulkan_rendering {

    ExtensionValidation::ExtensionValidation() {
        extensions_cache = std::set<const char*>();
    }

    void ExtensionValidation::populate(std::vector<VkExtensionProperties> extensions) {
        for (const auto& extension : extensions) {
            const char* ext_name = extension.extensionName;

            std::cout << "\t" << ext_name << std::endl;
            extensions_cache.insert(ext_name);
        }
    }

    bool ExtensionValidation::validate_glfw_extensions(const char** glfw_extensions) {
        bool are_contained = true;

        for (const char** ptr = glfw_extensions; *ptr; ++ptr) {
            if (extensions_cache.find(*ptr) == extensions_cache.end()) {
                std::cout << "Could not find: " << *ptr << std::endl;
                are_contained = false;
                break;
            }
        }

        return are_contained;
    }
}
