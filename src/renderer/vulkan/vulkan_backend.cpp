#include "odyssey.h"

using namespace Odyssey;

#include "vulkan_backend.h"
#include "vulkan_types.h"

VulkanBackend::~VulkanBackend()
{

}

bool VulkanBackend::Initialize(const RendererBackendConfig& config)
{
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

    unsigned int layerCount = 0;
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

    createInfo.enabledLayerCount = static_cast<uint32_t>(requiredValidationLayerNames.size());
    createInfo.ppEnabledLayerNames = requiredValidationLayerNames.data();

    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &myInstance));
    Logger::Log("Vulkan instance created");

#endif
    return true;
}
