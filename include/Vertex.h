#ifndef VERTEX_H
#define VERTEX_H

#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>

namespace vulkan_rendering {

    struct Vertex {
        glm::vec2 pos;
        glm::vec3 colour;

        /**
         * What does the vertex binding do?
         *
         * The vertex bindings describe the layout of the data. We can describe the # of bytes in between each entry and
         * if the data should move to the next entry after each instance.
         */
        static VkVertexInputBindingDescription get_binding_descriptions() {
            VkVertexInputBindingDescription bindings = {};

            // TODO: Implement and describe the data layout.

            return bindings;
        }
    };

    const std::vector<Vertex> vertices = {
        {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
    };

}

#endif
