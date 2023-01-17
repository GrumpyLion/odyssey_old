#pragma once

#include <vulkan/vulkan.h>

#include "odyssey/types.h"
#include "renderer/renderer_backend.h"
#include "VkBootstrap.h"

namespace Odyssey
{
    class VulkanBackend final : public RendererBackend
    {
    public:
        VulkanBackend();
        ~VulkanBackend() override;

        bool Initialize(const RendererBackendConfig& config) override;
        void InitStructures();

        bool CreateInstance();
        bool CreateDevice();
        bool CreateSwapchain(const RendererBackendConfig& config);

        void InitCommands();
    	void InitDefaultRenderPass();
        void InitFramebuffers(const RendererBackendConfig& config);

        void Render() override;

        VkCommandPoolCreateInfo CommandPoolCreateInfo(u32 queueFamilyIndex, VkCommandPoolCreateFlags flags = 0) const;
        VkCommandBufferAllocateInfo CommandBufferAllocateBuffer(VkCommandPool pool, u32 count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const;

    private:
        vkb::Instance myVKBInstance{};
        VkInstance myInstance{};
        VkSurfaceKHR mySurface{};
        VkDevice myDevice{};
        VkPhysicalDevice myPhysicalDevice{};
        VkPhysicalDeviceProperties myPhysicalDeviceProperties{};
        VkDebugUtilsMessengerEXT myDebugMessenger{};

    	VkSwapchainKHR mySwapchain{};
        VkFormat mySwapchainImageFormat{};
        Vector<VkImage> mySwapchainImages{};
        Vector<VkImageView> mySwapchainImageViews{};

        VkQueue myGraphicsQueue{};
        u32  myGraphicsQueueFamily{};

        VkCommandPool myCommandPool{};
        VkCommandBuffer myMainCommandBuffer{};

        VkRenderPass myRenderPass{};
        Vector<VkFramebuffer> myFramebuffers;

        VkSemaphore myPresentSemaphore;
        VkSemaphore myRenderSemaphore;
        VkFence myRenderFence;

        VkExtent2D myWindowExtent;

        // TEMP
        int myFrameNumber = 0;
    };

    namespace PlatformLayer
    {
	    VkSurfaceKHR GetVulkanSurface(VkInstance instance);
        bool GetVulkanExtensionNames(std::vector<const char*>& names);
    }
}