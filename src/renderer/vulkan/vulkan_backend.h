#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "vulkan_types.h"
#include "renderer/renderer_backend.h"
#include "VkBootstrap.h"

namespace Odyssey
{
    class VulkanBackend final : public RendererBackend
    {
    public:
        ~VulkanBackend() override;

        bool Initialize(const RendererBackendConfig& config) override;

        bool CreateInstance();
        bool CreateDevice();
        bool CreateSwapchain();

        vkb::Instance myInstance;
        VkSurfaceKHR mySurface;
        vkb::Device myDevice;
        vkb::Swapchain mySwapchain;
    };

    namespace PlatformLayer
    {
	    VkSurfaceKHR GetVulkanSurface(VkInstance instance);
        bool GetVulkanExtensionNames(std::vector<const char*>& names);
    }
}