#include "odyssey.h"

using namespace Odyssey;

#include <algorithm>

#include "vulkan_backend.h"
#include "vulkan_swapchain.h"
#include "vulkan_device.h"

void CreateSwapchain(VulkanBackend* backend, uint32_t width, uint32_t height, Swapchain& swapchain)
{
    VkExtent2D swapchainExtent = {width, height};

    bool found = false;
    for (uint32_t i = 0; i < backend->myVulkanDevice.mySwapchainSupport.myFormatsCount; ++i)
    {
        VkSurfaceFormatKHR format = backend->myVulkanDevice.mySwapchainSupport.myFormats[i];
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
        {
            swapchain.myImageFormat = format;
            found = true;
            break;
        }
    }

     if (!found)
        swapchain.myImageFormat = backend->myVulkanDevice.mySwapchainSupport.myFormats[0];

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < backend->myVulkanDevice.mySwapchainSupport.myPresentsCount; ++i)
    {
        VkPresentModeKHR mode = backend->myVulkanDevice.mySwapchainSupport.myPresentModes[i];
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            presentMode = mode;
            break;
        }
    }

    VulkanDevice::QuerySwapchainSupport(backend->myVulkanDevice.myPhysicalDevice, backend->mySurface, backend->myVulkanDevice.mySwapchainSupport);

    if (backend->myVulkanDevice.mySwapchainSupport.myCapabilities.currentExtent.width != UINT32_MAX)
        swapchainExtent = backend->myVulkanDevice.mySwapchainSupport.myCapabilities.currentExtent;

    const VkExtent2D min = backend->myVulkanDevice.mySwapchainSupport.myCapabilities.minImageExtent;
    const VkExtent2D max = backend->myVulkanDevice.mySwapchainSupport.myCapabilities.maxImageExtent;

    swapchainExtent.width = std::clamp(swapchainExtent.width, min.width, max.width);
    swapchainExtent.height = std::clamp(swapchainExtent.height, min.height, max.height);

    uint32_t imageCount = backend->myVulkanDevice.mySwapchainSupport.myCapabilities.minImageCount + 1;
    if (backend->myVulkanDevice.mySwapchainSupport.myCapabilities.maxImageCount > 0 && imageCount > backend->myVulkanDevice.mySwapchainSupport.myCapabilities.maxImageCount)
        imageCount = backend->myVulkanDevice.mySwapchainSupport.myCapabilities.maxImageCount;

    swapchain.myMaxFramesInFlight = imageCount - 1;

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchainCreateInfo.surface = backend->mySurface;
    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageFormat = swapchain.myImageFormat.format;
    swapchainCreateInfo.imageColorSpace = swapchain.myImageFormat.colorSpace;
    swapchainCreateInfo.imageExtent = swapchainExtent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


    if (backend->myVulkanDevice.myGraphicsQueueIndex != backend->myVulkanDevice.myPresentQueueIndex)
    {
        const uint32_t queueFamilyIndices[] =
        {
            (uint32_t)backend->myVulkanDevice.myGraphicsQueueIndex,
            (uint32_t)backend->myVulkanDevice.myPresentQueueIndex
        };

        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0;
        swapchainCreateInfo.pQueueFamilyIndices = nullptr;
    }

    swapchainCreateInfo.preTransform = backend->myVulkanDevice.mySwapchainSupport.myCapabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = true;
    swapchainCreateInfo.oldSwapchain = nullptr;

    VK_CHECK(vkCreateSwapchainKHR(backend->myVulkanDevice.myLogicalDevice, &swapchainCreateInfo, nullptr, &swapchain.mySwapchain));
}

void DestroySwapchain(VulkanBackend* backend, Swapchain& swapchain)
{
    vkDeviceWaitIdle(backend->myVulkanDevice.myLogicalDevice);

    vkDestroySwapchainKHR(backend->myVulkanDevice.myLogicalDevice, swapchain.mySwapchain, nullptr);
}

///////////////////////////////////////////////////////////////////////////////////////////

bool VulkanSwapchain::Initialize(VulkanBackend* backend, int width, int height)
{
    CreateSwapchain(backend, width, height, backend->myVulkanSwapchain);

    return true;
}

void VulkanSwapchain::Shutdown(VulkanBackend* backend)
{
    DestroySwapchain(backend, backend->myVulkanSwapchain);
}

void VulkanSwapchain::Recreate(VulkanBackend* backend, int width, int height)
{
    DestroySwapchain(backend, backend->myVulkanSwapchain);
    CreateSwapchain(backend, width, height, backend->myVulkanSwapchain);
}

bool VulkanSwapchain::AcquireNextImageIndex(VulkanBackend* backend, long timeoutNS, VkSemaphore imageAvailableSemaphore, VkFence fence, uint32_t* outImageIndex)
{
    return true;
}

void VulkanSwapchain::SwapchainPresent(VulkanBackend* backend, VkQueue graphicsQueue, VkQueue presentQueue, VkSemaphore renderCompleteSemaphore, uint32_t presentImageIndex)
{

}