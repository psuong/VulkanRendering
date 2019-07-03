#ifndef VERTEX_H
#define VERTEX_H

#include <glm/glm.hpp>
#include <vector>

namespace vulkan_rendering {

    struct Vertex {
        glm::vec2 pos;
        glm::vec3 colour;
    };

    const std::vector<Vertex> vertices = {
        {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
    };

}

#endif
