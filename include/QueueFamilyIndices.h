#ifndef QUEUE_FAMILY_INDICES_H
#define QUEUE_FAMILY_INDICES_H

#include <optional>

namespace vulkan_rendering {

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphics_family;

        bool is_complete() {
            return graphics_family.has_value();
        }
    };
}

#endif
