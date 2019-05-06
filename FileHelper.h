#ifndef FILE_HELPER_H
#define FILE_HELPER_H

#include <fstream>
#include <string>
#include <vector>

namespace vulkan_rendering {

    static std::vector<char> Read_File(const std::string& file_name) {

        // ate: start at the end of the file and read it as a binary file.
        std::ifstream file(file_name, std::ios::ate | std::ios::binary);

        if (!file.is_open) {
            throw std::runtime_error("Failed to open file!");
        }

        auto file_size = (size_t)file.tellg();
        std::vector<char> buffer(file_size);
        file.seekg(0);
        file.read(buffer.data(), file_size);
        file.close();

        return buffer;
    }
}

#endif
