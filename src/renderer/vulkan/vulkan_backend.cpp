#include "odyssey.h"

using namespace Odyssey;

#include "vulkan_backend.h"

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData)
{
    switch (messageSeverity) 
    {
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            Logger::LogError(callbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            Logger::Log(callbackData->pMessage);
            break;
    }
    return VK_FALSE;
}

VulkanBackend::~VulkanBackend()
{
    vkb::destroy_swapchain(mySwapchain);
    vkb::destroy_surface(myInstance, mySurface);
#if IS_DEBUG
    vkb::destroy_debug_utils_messenger(myInstance, myInstance.debug_messenger);
#endif
	vkb::destroy_device(myDevice);
    vkb::destroy_instance(myInstance);
}

bool VulkanBackend::Initialize(const RendererBackendConfig& config)
{
    if (!CreateInstance())
        return false;

    if (!CreateDevice())
        return false;

    if (!CreateSwapchain())
        return false;

    return true;
}

bool VulkanBackend::CreateInstance()
{
	vkb::InstanceBuilder instanceBuilder;

	constexpr uint32_t logSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT  |
						         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT    |
						         VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;

	auto instanceReturn = instanceBuilder
#if IS_DEBUG
										.set_debug_callback(VulkanDebugCallback)
										.request_validation_layers()
										.set_debug_messenger_severity(logSeverity)
#endif
										.build();
	if (!instanceReturn)
    {
		Logger::LogError(instanceReturn.error().message());
		return false;
	}
	myInstance = instanceReturn.value();
	mySurface = PlatformLayer::GetVulkanSurface(myInstance);    

    return true;
}

bool VulkanBackend::CreateDevice()
{
    vkb::PhysicalDeviceSelector physDeviceSelector(myInstance);
    const auto physDeviceReturn = physDeviceSelector.set_surface(mySurface).select();
	if (!physDeviceReturn)
    {
		Logger::LogError(physDeviceReturn.error().message());
		return false;
	}

	const vkb::PhysicalDevice physicalDevice = physDeviceReturn.value();
    const vkb::DeviceBuilder deviceBuilder{ physicalDevice };
	auto deviceReturn = deviceBuilder.build ();
	if (!deviceReturn) {
		Logger::LogError(deviceReturn.error ().message());
		return false;
	}

	myDevice = deviceReturn.value ();

    return true;
}

bool VulkanBackend::CreateSwapchain()
{
    vkb::SwapchainBuilder swapchain_builder{ myDevice };
	const auto swapchainReturn = swapchain_builder.set_old_swapchain(mySwapchain).build ();
	if (!swapchainReturn) 
    {
		Logger::LogError("{} {}", swapchainReturn.error ().message (), swapchainReturn.vk_result ());
        return false;
	}
    vkb::destroy_swapchain(mySwapchain);
	mySwapchain = swapchainReturn.value();

    return true;
}