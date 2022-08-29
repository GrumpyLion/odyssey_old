#pragma once

#include <vulkan/vulkan.h>

namespace Odyssey
{
    class VulkanBackend;

    namespace VulkanSwapchain
    {
        bool Initialize(VulkanBackend* backend, int width, int height);
        void Shutdown(VulkanBackend* backend);

        void Recreate(VulkanBackend* backend, int width, int height);

        bool AcquireNextImageIndex(VulkanBackend* backend, long timeoutNS, VkSemaphore imageAvailableSemaphore, VkFence fence, uint32_t* outImageIndex);
        void SwapchainPresent(VulkanBackend* backend, VkQueue graphicsQueue, VkQueue presentQueue, VkSemaphore renderCompleteSemaphore, uint32_t presentImageIndex);
    }
}