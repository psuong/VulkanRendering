#ifndef SWAP_CHAIN_SUPPORT_DETAILS_H
#define SWAP_CHAIN_SUPPORT_DETAILS_H

#include <vector>
#include <vulkan/vulkan.h>

namespace vulkan_rendering {

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> present_modes;
    };
}

#endif
