#include "odyssey.h"

using namespace Odyssey;

#include "vulkan_device.h"
#include "vulkan_backend.h"

struct PhysicalDeviceRequirements
{
    bool myGraphics{};
    bool myPresent{};
    bool myCompute{};
    bool myTransfer{};
    bool mySamplerAnisotropy{};
    bool myDiscreteGPU{};
    std::vector<std::string> myDeviceExtensionNames;
};

struct PhysicalDeviceQueueFamilyInfo
{
    uint32_t myGraphicsFamilyIndex{};
    uint32_t myPresentFamilyIndex{};
    uint32_t myComputeFamilyIndex{};
    uint32_t myTransferFamilyIndex{};
};

void QuerySwapchainSupport(
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface,
    VulkanSwapchainSupportInfo& outSupportInfo) 
{
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physicalDevice,
        surface,
        &outSupportInfo.myCapabilities));

    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
        physicalDevice,
        surface,
        &outSupportInfo.myFormatsCount,
        nullptr));

    if (outSupportInfo.myFormatsCount != 0) 
    {
        outSupportInfo.myFormats.resize(outSupportInfo.myFormatsCount);
    
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice,
            surface,
            &outSupportInfo.myFormatsCount,
            outSupportInfo.myFormats.data()));
    }

    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDevice,
        surface,
        &outSupportInfo.myPresentsCount,
        0));
    
    if (outSupportInfo.myPresentsCount != 0) {
        outSupportInfo.myPresentModes.resize(outSupportInfo.myPresentsCount);

        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice,
            surface,
            &outSupportInfo.myPresentsCount,
            outSupportInfo.myPresentModes.data()));
    }
}

bool PhysicalDeviceMeetsRequirements(VkPhysicalDevice device, 
    VkSurfaceKHR surface,
    const VkPhysicalDeviceProperties& properties,
    const VkPhysicalDeviceFeatures& features,
    const PhysicalDeviceRequirements& requirements,
    PhysicalDeviceQueueFamilyInfo& outQueueFamilyInfo,
    VulkanSwapchainSupportInfo& outSwapchainSupport)
{
    outQueueFamilyInfo.myGraphicsFamilyIndex = -1;
    outQueueFamilyInfo.myPresentFamilyIndex = -1;
    outQueueFamilyInfo.myComputeFamilyIndex = -1;
    outQueueFamilyInfo.myTransferFamilyIndex = -1;

    if (requirements.myDiscreteGPU && properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        Logger::Log("Physical Device {} is not a discrete GPU. Skipping..", properties.deviceName);
        return false;
    }

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    VkQueueFamilyProperties queueFamilies[32];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

    int minTransferScore = 999;
    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        int currentTransferScore = 0;
        if (outQueueFamilyInfo.myGraphicsFamilyIndex == -1 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            outQueueFamilyInfo.myGraphicsFamilyIndex = i;
            currentTransferScore++;

            VkBool32 supportsPresent = false;
            VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supportsPresent));
            if (supportsPresent)
            {
                outQueueFamilyInfo.myPresentFamilyIndex = i;
                currentTransferScore++;
            }        
        }

        if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            outQueueFamilyInfo.myComputeFamilyIndex = i;
            currentTransferScore++;
        }

        if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            if (currentTransferScore <= minTransferScore)
            {
                minTransferScore = currentTransferScore;
                outQueueFamilyInfo.myTransferFamilyIndex = i;
            }
        }
    }

    if (outQueueFamilyInfo.myPresentFamilyIndex == -1)
    {
        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            VkBool32 supportsPresent = false;
            VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supportsPresent));
            if (supportsPresent)
            {
                outQueueFamilyInfo.myPresentFamilyIndex = i;
                if (outQueueFamilyInfo.myPresentFamilyIndex != outQueueFamilyInfo.myGraphicsFamilyIndex)
                {
                    Logger::Log("Warning: Different queue index use for present vs graphics {}", i);
                }
                break;
            }
        }
    }

    Logger::Log("{} |   {} |   {} |   {} |  {}",
        outQueueFamilyInfo.myGraphicsFamilyIndex != -1,
        outQueueFamilyInfo.myPresentFamilyIndex != -1,
        outQueueFamilyInfo.myComputeFamilyIndex != -1,
        outQueueFamilyInfo.myTransferFamilyIndex != -1,
        properties.deviceName);

    if (
        (!requirements.myGraphics || (requirements.myGraphics && outQueueFamilyInfo.myGraphicsFamilyIndex != -1)) &&
        (!requirements.myPresent || (requirements.myPresent && outQueueFamilyInfo.myPresentFamilyIndex != -1)) &&
        (!requirements.myCompute || (requirements.myCompute && outQueueFamilyInfo.myComputeFamilyIndex != -1)) &&
        (!requirements.myTransfer || (requirements.myTransfer && outQueueFamilyInfo.myTransferFamilyIndex != -1))) {
        Logger::Log("Device meets queue requirements.");
        Logger::Log("Graphics Family Index: {}", outQueueFamilyInfo.myGraphicsFamilyIndex);
        Logger::Log("Present Family Index:  {}", outQueueFamilyInfo.myPresentFamilyIndex);
        Logger::Log("Transfer Family Index: {}", outQueueFamilyInfo.myTransferFamilyIndex);
        Logger::Log("Compute Family Index:  {}", outQueueFamilyInfo.myComputeFamilyIndex);

        QuerySwapchainSupport(device, surface, outSwapchainSupport);

        if (outSwapchainSupport.myFormatsCount < 1 || outSwapchainSupport.myPresentsCount < 1)
        {
            outSwapchainSupport.myFormats.clear();
            outSwapchainSupport.myPresentModes.clear();

            Logger::Log("Required swapchain support not found. Skipping...");
            return false;
        }

        if (!requirements.myDeviceExtensionNames.empty())
        {
            uint32_t availableExtensionCount = 0;
            std::vector<VkExtensionProperties> availableExtensions{};

            VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &availableExtensionCount, nullptr));

            if (availableExtensionCount != 0)
            {
                availableExtensions.resize(availableExtensionCount);
                VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &availableExtensionCount, availableExtensions.data()));

                uint32_t requiredExtensionCount = static_cast<uint32_t>(requirements.myDeviceExtensionNames.size());
                for (uint32_t i = 0; i < requiredExtensionCount; ++i)
                {
                    bool found = false;
                    for (uint32_t j = 0; j < availableExtensionCount; ++j)
                    {
                        if (requirements.myDeviceExtensionNames[i].compare(availableExtensions[j].extensionName) == 0)
                        {
                            found = true;
                            break;
                        }
                    }

                    if (!found)
                    {
                        Logger::Log("Required extension not found: {}. Skipping...", requirements.myDeviceExtensionNames[i]);
                        return false;
                    }
                }
            }
        }

        if (requirements.mySamplerAnisotropy && !features.samplerAnisotropy)
        {
            Logger::Log("Device does not support anisotropy. Skipping...");
            return false;
        }

        return true;
    }

    return false;
}

bool SelectPhysicalDevice(VulkanBackend* backend)
{
    uint32_t physicalDeviceCount = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(backend->myInstance, &physicalDeviceCount, nullptr));
    if (physicalDeviceCount == 0)
    {
        Logger::LogError("Can't find any physical devices that support vulkan!");
        return false;
    }

    VkPhysicalDevice physicalDevices[32];
    VK_CHECK(vkEnumeratePhysicalDevices(backend->myInstance, &physicalDeviceCount, physicalDevices));
    for (const auto& physicalDevice : physicalDevices)
    {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);

        VkPhysicalDeviceFeatures features{};
        vkGetPhysicalDeviceFeatures(physicalDevice, &features);

        VkPhysicalDeviceMemoryProperties memory{};
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memory);

        Logger::Log("Evaluating device: {}.", properties.deviceName);

        bool supportsDeviceLocalHostVisible = false;
        for (uint32_t i = 0; i < memory.memoryTypeCount; ++i) 
        {
            // Check each memory type to see if its bit is set to 1.
            if (((memory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) &&
                 (memory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0)
            {
                supportsDeviceLocalHostVisible = true;
                break;
            }
        }

        // TODO: These requirements should probably be driven by engine
        // configuration.
        PhysicalDeviceRequirements requirements{};
        requirements.myGraphics = true;
        requirements.myPresent = true;
        requirements.myTransfer = true;
        // NOTE: Enable this if compute will be required.
        // requirements.compute = true;
        requirements.mySamplerAnisotropy = true;
        requirements.myDiscreteGPU = true;

        requirements.myDeviceExtensionNames.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        PhysicalDeviceQueueFamilyInfo queueInfo{};
        bool result = PhysicalDeviceMeetsRequirements(
            physicalDevice, backend->mySurface, properties, features, requirements, queueInfo, backend->myVulkanDevice.mySwapchainSupport);
    
        if (result)
        {
            Logger::Log("Selected Device: {}", properties.deviceName);

            switch (properties.deviceType)
            {
                default:
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    Logger::Log("GPU type is unknown");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    Logger::Log("GPU type is integrated");
                    break;                    
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    Logger::Log("GPU type is discrete");
                    break;                    
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    Logger::Log("GPU type is virtual");
                    break;                    
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    Logger::Log("GPU type is cpu");
                    break;
            }

            Logger::Log("GPU Driver version: {}.{}.{}", 
                VK_VERSION_MAJOR(properties.driverVersion),
                VK_VERSION_MINOR(properties.driverVersion),
                VK_VERSION_PATCH(properties.driverVersion));
            
            Logger::Log("Vulkan API version: {}.{}.{}",
                VK_VERSION_MAJOR(properties.apiVersion),
                VK_VERSION_MINOR(properties.apiVersion),
                VK_VERSION_PATCH(properties.apiVersion));

            for (uint32_t i = 0; i < memory.memoryHeapCount; ++i) 
            {
                float memorySizeGB = (((float)memory.memoryHeaps[i].size) / 1024.0f / 1024.0f / 1024.0f);
                if (memory.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                    Logger::Log("Local GPU memory: {:.2f} GiB", memorySizeGB);
                } else {
                    Logger::Log("Shared System memory: {:.2f} GiB", memorySizeGB);
                }
            }

            backend->myVulkanDevice.myPhysicalDevice= physicalDevice;
            backend->myVulkanDevice.myGraphicsQueueIndex = queueInfo.myGraphicsFamilyIndex;
            backend->myVulkanDevice.myPresentQueueIndex = queueInfo.myPresentFamilyIndex;
            backend->myVulkanDevice.myTransferQueueIndex = queueInfo.myTransferFamilyIndex;
            backend->myVulkanDevice.myComputeQueueIndex = queueInfo.myComputeFamilyIndex;

            backend->myVulkanDevice.myProperties = properties;
            backend->myVulkanDevice.myFeatures = features;
            backend->myVulkanDevice.myMemory = memory;
            backend->myVulkanDevice.mySupportsDeviceLocalHostVisible = supportsDeviceLocalHostVisible;
            break;
        }
    }

    if (!backend->myVulkanDevice.myPhysicalDevice)
    {
        Logger::LogError("No physical device were found with matching requirements");
        return false;
    }

    Logger::Log("Physical device selected");

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////

bool VulkanDevice::Initialize(VulkanBackend* backend)
{
    if (!SelectPhysicalDevice(backend))
        return false;

    bool presentSharesGraphicsQueue = backend->myVulkanDevice.myGraphicsQueueIndex == backend->myVulkanDevice.myPresentQueueIndex;
    bool transferSharesGraphicsQueue = backend->myVulkanDevice.myGraphicsQueueIndex == backend->myVulkanDevice.myTransferQueueIndex; 
    int indexCount = 1;
    if (!presentSharesGraphicsQueue)
        indexCount++;
    
    if (!transferSharesGraphicsQueue)
        indexCount++;

    uint32_t indices[32];
    int index = 0;
    indices[index++] = backend->myVulkanDevice.myGraphicsQueueIndex;
    if (!presentSharesGraphicsQueue)
        indices[index++] = backend->myVulkanDevice.myPresentQueueIndex;

    if (!transferSharesGraphicsQueue)
        indices[index++] = backend->myVulkanDevice.myTransferQueueIndex;

    VkDeviceQueueCreateInfo queueCreateInfos[32];
    for (int i = 0; i < indexCount; ++i)
    {
        queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[i].queueFamilyIndex = indices[i];
        queueCreateInfos[i].queueCount = 1;

        // TODO: Enable this for a future enhancement.
        // if (indices[i] == context->device.graphics_queue_index) {
        //     queue_create_infos[i].queueCount = 2;
        // }

        queueCreateInfos[i].flags = 0;
        queueCreateInfos[i].pNext = nullptr;
        float queuePriority = 1.0f;
        queueCreateInfos[i].pQueuePriorities = &queuePriority;
    }

    // Request device features.
    // TODO: should be config driven
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = true;

    bool protabilityRequired = false;
    uint32_t availableExtensionCount = 0;
    std::vector<VkExtensionProperties> availableExtensions{};
    VK_CHECK(vkEnumerateDeviceExtensionProperties(backend->myVulkanDevice.myPhysicalDevice, nullptr, &availableExtensionCount, nullptr));
    if (availableExtensionCount != 0)
    {
        availableExtensions.resize(availableExtensionCount);
        VK_CHECK(vkEnumerateDeviceExtensionProperties(backend->myVulkanDevice.myPhysicalDevice, nullptr, &availableExtensionCount, availableExtensions.data()));
        for (uint32_t i = 0; i < availableExtensionCount; ++i)
        {
            if (strcmp(availableExtensions[i].extensionName, "VK_KHR_portability_subset") == 0)
            {
                Logger::Log("Adding required extension VK_KHR_portability_subset");
                protabilityRequired = true;
                break;
            }
        }
    }

    int extensionCount = protabilityRequired ? 2 : 1;
    const char* extensionNames[2];
    extensionNames[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    extensionNames[1] = "VK_KHR_portability_subset";

    VkDeviceCreateInfo deviceCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceCreateInfo.queueCreateInfoCount = indexCount;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = extensionCount;
    deviceCreateInfo.ppEnabledExtensionNames = extensionNames;

    // Deprecated and ignored, so pass nothing.
    deviceCreateInfo.enabledLayerCount = 0;
    deviceCreateInfo.ppEnabledLayerNames = 0;

    VK_CHECK(vkCreateDevice(
        backend->myVulkanDevice.myPhysicalDevice,
        &deviceCreateInfo,
        nullptr,
        &backend->myVulkanDevice.myLogicalDevice));

    Logger::Log("Logical device created");

    vkGetDeviceQueue(
        backend->myVulkanDevice.myLogicalDevice,
        backend->myVulkanDevice.myGraphicsQueueIndex,
        0,
        &backend->myVulkanDevice.myGraphicsQueue);

    vkGetDeviceQueue(
        backend->myVulkanDevice.myLogicalDevice,
        backend->myVulkanDevice.myPresentQueueIndex,
        0,
        &backend->myVulkanDevice.myPresentQueue);

    vkGetDeviceQueue(
        backend->myVulkanDevice.myLogicalDevice,
        backend->myVulkanDevice.myTransferQueueIndex,
        0,
        &backend->myVulkanDevice.myTransferQueue);

    Logger::Log("Queues obtained");

    VkCommandPoolCreateInfo poolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolCreateInfo.queueFamilyIndex = backend->myVulkanDevice.myGraphicsQueueIndex;
    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(vkCreateCommandPool(
        backend->myVulkanDevice.myLogicalDevice,
        &poolCreateInfo,
        nullptr,
        &backend->myGraphicsCommandPool));

    Logger::Log("Graphics command pool created.");

    return true;
}

void VulkanDevice::Shutdown(VulkanBackend* backend)
{
    backend->myVulkanDevice.myGraphicsQueue = {};
    backend->myVulkanDevice.myPresentQueue = {};
    backend->myVulkanDevice.myTransferQueue = {};

    vkDestroyCommandPool(backend->myVulkanDevice.myLogicalDevice, backend->myGraphicsCommandPool, nullptr);

    if (backend->myVulkanDevice.myLogicalDevice)
    {
        vkDestroyDevice(backend->myVulkanDevice.myLogicalDevice, nullptr);
        backend->myVulkanDevice.myLogicalDevice = {};
    }

    backend->myVulkanDevice.myPhysicalDevice = {};

    backend->myVulkanDevice.myGraphicsQueueIndex = -1;
    backend->myVulkanDevice.myTransferQueueIndex = -1;
    backend->myVulkanDevice.myPresentQueueIndex = -1;
}

void VulkanDevice::QuerySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VulkanSwapchainSupportInfo& supportInfo)
{
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physicalDevice,
        surface,
        &supportInfo.myCapabilities));

    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
        physicalDevice,
        surface,
        &supportInfo.myFormatsCount,
        0));

    if (supportInfo.myFormatsCount != 0)
    {
        supportInfo.myFormats.resize(supportInfo.myFormatsCount);

        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice,
            surface,
            &supportInfo.myFormatsCount,
            supportInfo.myFormats.data()));
    }

    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDevice,
        surface,
        &supportInfo.myPresentsCount,
        0));

    if (supportInfo.myPresentsCount != 0) 
    {
        supportInfo.myPresentModes.resize(supportInfo.myPresentsCount);
        
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice,
            surface,
            &supportInfo.myPresentsCount,
            supportInfo.myPresentModes.data()));
    }
}

bool VulkanDevice::DetectDepthFormat(VulkanBackend* backend)
{
    const int candidateCount = 3;
    VkFormat candidates[3] = 
    {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    int sizes[3] = {4, 4, 3};

    uint32_t flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for (int i = 0; i < candidateCount; ++i) 
    {
        VkFormatProperties properties{};
        vkGetPhysicalDeviceFormatProperties(backend->myVulkanDevice.myPhysicalDevice, candidates[i], &properties);

        if ((properties.linearTilingFeatures & flags) == flags)
        {
            backend->myVulkanDevice.myDepthFormat = candidates[i];
            backend->myVulkanDevice.myDepthChannelCount = sizes[i];
            return true;
        } 
        else if ((properties.optimalTilingFeatures & flags) == flags)
        {
            backend->myVulkanDevice.myDepthFormat = candidates[i];
            backend->myVulkanDevice.myDepthChannelCount = sizes[i];
            return true;
        }
    }

    return false;
}