#ifndef FILE_HELPER_H
#define FILE_HELPER_H

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace vulkan_rendering {
    
    static std::vector<char> read_file(const std::string& filename) {
        std::cout << "Loading shader path: " << filename << std::endl;
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file!" + filename);
        }

        size_t file_size = (size_t)file.tellg();
        std::vector<char> buffer(file_size);

        file.seekg(0);
        file.read(buffer.data(), file_size);
        file.close();
        return buffer;
    }
}

#endif
