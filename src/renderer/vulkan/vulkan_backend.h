#pragma once

#include "vulkan/vulkan.h"
#include "renderer/renderer_backend.h"
#include <vector>

namespace Odyssey
{
    class VulkanBackend final : public RendererBackend
    {
    public:
        ~VulkanBackend();

        bool Initialize(const RendererBackendConfig& config) override;

    private:
        VkInstance myInstance{};
    };

    namespace PlatformLayer
    {
	    VkSurfaceKHR GetVulkanSurface(VkInstance instance);
        bool GetVulkanExtensionNames(std::vector<const char*>& names);
    }
}