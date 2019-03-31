#ifndef QUEUE_FAMILY_DEVICE_H
#define QUEUE_FAMILY_DEVICE_H

#include <optional>

namespace vulkan_rendering {
	
	struct QueueFamilyDevice {
		std::optional<uint32_t> graphics_family;

		bool inline is_complete() {
			return graphics_family.has_value();
		}
	};
}

#endif