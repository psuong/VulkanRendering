#ifndef VERTEX_H
#define VERTEX_H

#include <array>
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
            VkVertexInputBindingDescription binding_descriptions = {};
            binding_descriptions.binding                         = 0;
            binding_descriptions.stride                          = sizeof(Vertex);
            binding_descriptions.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;
            return binding_descriptions;
        }

        /**
         * Binding tells Vulkan from which binding the per vertex data comes in. The location param references the
         * location directive of the input in the vertex shader.
         *
         * The input in the vertex shader with location 0 is the position, which has 2 32 bit floats.
         *
         * Formats can be described with the following:
         * float: VK_FORMAT_R32_SFLOAT
         * vec2: VK_FORMAT_R32G32_SFLOAT
         * vec3: VK_FORMAT_R32G32B32_SFLOAT
         * vec4: VK_FORMAT_R32G32B32A32_SFLOAT
         */
        static std::array<VkVertexInputAttributeDescription, 2> get_attribute_descriptions() {
            std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions = {};

            attribute_descriptions[0].binding  = 0;
            attribute_descriptions[0].location = 0;
            attribute_descriptions[0].format   = VK_FORMAT_R32G32_SFLOAT;
            attribute_descriptions[0].offset   = offsetof(Vertex, pos);

            attribute_descriptions[1].binding  = 0;
            attribute_descriptions[1].location = 1;
            attribute_descriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
            attribute_descriptions[1].offset   = offsetof(Vertex, colour);

            return attribute_descriptions;
        }
    };

    const std::vector<Vertex> vertices = {
        {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
    };

}

#endif
