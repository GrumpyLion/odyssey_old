#include "odyssey.h"

using namespace Odyssey;

#include "vulkan_backend.h"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"

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
    vkDeviceWaitIdle(myVulkanDevice.myLogicalDevice);

    VulkanSwapchain::Shutdown(this);
    VulkanDevice::Shutdown(this);

    vkDestroySurfaceKHR(myInstance, mySurface, nullptr);

#if IS_DEBUG
    const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(myInstance, "vkDestroyDebugUtilsMessengerEXT"));
    func(myInstance, myDebugMessenger, nullptr);
#endif
    vkDestroyInstance(myInstance, nullptr);
}

bool VulkanBackend::Initialize(const RendererBackendConfig& config)
{
    myFramebufferWidth  = 1024;
    myFramebufferHeight = 600;
    
    VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.apiVersion = VK_API_VERSION_1_2;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Odyssey";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    createInfo.pApplicationInfo = &appInfo;

    std::vector<const char*> requiredExtensions;
    requiredExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    PlatformLayer::GetVulkanExtensionNames(requiredExtensions);

#if IS_DEBUG
    requiredExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    Logger::Log("Vulkan Extensions");
    for (const auto& extension : requiredExtensions)
    {
        Logger::Log(extension);
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    std::vector<const char*> requiredValidationLayerNames;

#if IS_DEBUG
    requiredValidationLayerNames.push_back("VK_LAYER_KHRONOS_validation");

    uint32_t layerCount = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, 0));
    std::vector<VkLayerProperties> availableLayers(layerCount);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()));

    for (const auto& requiredLayer : requiredValidationLayerNames)
    {
        Logger::Log("Searching for layer: {}", requiredLayer);
        bool found = false;
	    for (const auto& availableLayer : availableLayers)
	    {
		    if (strcmp(requiredLayer, availableLayer.layerName) == 0)
		    {
                found = true;
                Logger::Log("Found");
                break;
		    }
	    }

        if (!found)
        {
            Logger::LogError("Required validation layer is missing: {}", requiredLayer);
            return false;
        }
    }
#endif

    createInfo.enabledLayerCount = static_cast<uint32_t>(requiredValidationLayerNames.size());
    createInfo.ppEnabledLayerNames = requiredValidationLayerNames.data();

    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &myInstance));
    Logger::Log("Vulkan instance created");

#if IS_DEBUG
    Logger::Log("Creating Vulkan Debugger");
    uint32_t logSeverity =  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT   |
                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;


    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    debugCreateInfo.messageSeverity = logSeverity;
    debugCreateInfo.messageType =   VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT     |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | 
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

    debugCreateInfo.pfnUserCallback = VulkanDebugCallback;

    const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(myInstance, "vkCreateDebugUtilsMessengerEXT"));
    ASSERT_MSG(func, "Failed to create debug messenger!");
    VK_CHECK(func(myInstance, &debugCreateInfo, nullptr, &myDebugMessenger));
    Logger::Log("Vulkan debugger created.");
#endif

    Logger::Log("Creating Vulkan Surface");
    mySurface = PlatformLayer::GetVulkanSurface(myInstance);
    if (!mySurface)
    {
        Logger::LogError("Failed to create Vulkan surface");
        return false;
    }

    if (!VulkanDevice::Initialize(this))
        return false;

    if (!VulkanSwapchain::Initialize(this, myFramebufferWidth, myFramebufferHeight))
        return false;
    
    return true;
}