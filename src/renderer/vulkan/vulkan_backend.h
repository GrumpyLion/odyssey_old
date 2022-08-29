#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "vulkan_types.h"
#include "renderer/renderer_backend.h"

namespace Odyssey
{
    struct Device
    {        
        VkDevice myLogicalDevice{};
        VkPhysicalDevice myPhysicalDevice{};

        int myGraphicsQueueIndex{};
        int myPresentQueueIndex{};
        int myTransferQueueIndex{};
        int myComputeQueueIndex{};

        VkQueue myGraphicsQueue{};
        VkQueue myPresentQueue{};
        VkQueue myTransferQueue{};

        bool mySupportsDeviceLocalHostVisible{};

        VkPhysicalDeviceProperties myProperties{};
        VkPhysicalDeviceFeatures myFeatures{};
        VkPhysicalDeviceMemoryProperties myMemory{};
    
        VkFormat myDepthFormat{};
        int myDepthChannelCount{};

        VulkanSwapchainSupportInfo mySwapchainSupport{};
    };

    struct Swapchain
    {
        VkSurfaceFormatKHR myImageFormat{};
        VkSwapchainKHR mySwapchain{};

        int myMaxFramesInFlight{};
        uint32_t myImageCount{};
    };

    class VulkanBackend final : public RendererBackend
    {
    public:
        ~VulkanBackend();

        bool Initialize(const RendererBackendConfig& config) override;

        int myFramebufferWidth{};
        int myFramebufferHeight{};

        VkInstance myInstance{};
        VkSurfaceKHR mySurface{};

        Device myVulkanDevice{};
        Swapchain myVulkanSwapchain{};

        VkCommandPool myGraphicsCommandPool{};

#if IS_DEBUG
        VkDebugUtilsMessengerEXT myDebugMessenger{};
#endif
    };

    namespace PlatformLayer
    {
	    VkSurfaceKHR GetVulkanSurface(VkInstance instance);
        bool GetVulkanExtensionNames(std::vector<const char*>& names);
    }
}