#pragma once

#include <vulkan/vulkan.h>
#include "vulkan_types.h"

namespace Odyssey
{
    class VulkanBackend;

    namespace VulkanDevice
    {
        bool Initialize(VulkanBackend* vulkanBackend);
        void Shutdown(VulkanBackend* vulkanBackend);

        void QuerySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VulkanSwapchainSupportInfo& supportInfo);
        bool DetectDepthFormat(VulkanBackend* vulkanBackend);
    };
}