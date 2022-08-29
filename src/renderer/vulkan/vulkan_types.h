#pragma once

#include "odyssey.h"
#include <vulkan/vulkan.h>

#define VK_CHECK(expr)                  \
    {                                   \
        ASSERT(expr == VK_SUCCESS);     \
    }


struct VulkanSwapchainSupportInfo 
{
    VkSurfaceCapabilitiesKHR myCapabilities{};
    uint32_t myFormatsCount{};
    std::vector<VkSurfaceFormatKHR> myFormats{};
    
    uint32_t myPresentsCount{};
    std::vector<VkPresentModeKHR> myPresentModes{};
};