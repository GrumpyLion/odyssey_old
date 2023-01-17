#include "odyssey.h"

using namespace Odyssey;

#include "vulkan_types.h"
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
            Logger::LogWarn(callbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            Logger::Log(callbackData->pMessage);
            break;
    }
    return VK_FALSE;
}

VulkanBackend::VulkanBackend()
{

}

VulkanBackend::~VulkanBackend()
{
    vkDeviceWaitIdle(myDevice);

    vkDestroySemaphore(myDevice, myPresentSemaphore, nullptr);
    vkDestroySemaphore(myDevice, myRenderSemaphore, nullptr);
    vkDestroyFence(myDevice, myRenderFence, nullptr);

    vkDestroyCommandPool(myDevice, myCommandPool, nullptr);

    vkDestroySwapchainKHR(myDevice, mySwapchain, nullptr);
    vkDestroyRenderPass(myDevice, myRenderPass, nullptr);

    for (int i = 0; i < myFramebuffers.size(); ++i)
    {
        vkDestroyFramebuffer(myDevice, myFramebuffers[i], nullptr);
        vkDestroyImageView(myDevice, mySwapchainImageViews[i], nullptr);
    }

    vkDestroyDevice(myDevice, nullptr);
    vkDestroySurfaceKHR(myInstance, mySurface, nullptr);
#if IS_DEBUG
    vkb::destroy_debug_utils_messenger(myInstance, myDebugMessenger);
#endif
    vkDestroyInstance(myInstance, nullptr);
}

bool VulkanBackend::Initialize(const RendererBackendConfig& config)
{
    if (!CreateInstance())
        return false;

    if (!CreateDevice())
        return false;

    if (!CreateSwapchain(config))
        return false;

    InitCommands();
    InitDefaultRenderPass();
    InitFramebuffers(config);
    InitStructures();

    return true;
}

void VulkanBackend::InitStructures()
{
    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_CHECK(vkCreateFence(myDevice, &fenceCreateInfo, nullptr, &myRenderFence));

    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VK_CHECK(vkCreateSemaphore(myDevice, &semaphoreCreateInfo, nullptr, &myPresentSemaphore));
    VK_CHECK(vkCreateSemaphore(myDevice, &semaphoreCreateInfo, nullptr, &myRenderSemaphore));
}

bool VulkanBackend::CreateInstance()
{
	vkb::InstanceBuilder instanceBuilder;

	constexpr uint32_t logSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT  |
#if VERY_VERBOSE
								 VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT    |
#endif
						         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT    |
						         VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;

	auto instanceReturn = instanceBuilder
										.require_api_version(1,1,0)
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

    myVKBInstance = instanceReturn.value();
	myInstance = myVKBInstance.instance;
    myDebugMessenger = myVKBInstance.debug_messenger;
	mySurface = PlatformLayer::GetVulkanSurface(myInstance);    

    return true;
}

bool VulkanBackend::CreateDevice()
{
    vkb::PhysicalDeviceSelector physDeviceSelector(myVKBInstance);
    const auto physDeviceReturn = physDeviceSelector.set_surface(mySurface).set_minimum_version(1, 1).select();
	if (!physDeviceReturn)
    {
		Logger::LogError(physDeviceReturn.error().message());
		return false;
	}

	const vkb::PhysicalDevice physicalDevice = physDeviceReturn.value();
    const vkb::DeviceBuilder deviceBuilder{ physicalDevice };
	auto deviceReturn = deviceBuilder.build();
	if (!deviceReturn) {
		Logger::LogError(deviceReturn.error().message());
		return false;
	}

    vkb::Device vkbDevice = deviceReturn.value();

	myDevice = vkbDevice;
    myPhysicalDevice = physicalDevice.physical_device;
    myPhysicalDeviceProperties = physicalDevice.properties;
	myGraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    myGraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    return true;
}

bool VulkanBackend::CreateSwapchain(const RendererBackendConfig& config)
{
    vkb::SwapchainBuilder swapchain_builder{ myPhysicalDevice, myDevice, mySurface };
	const auto swapchainReturn = swapchain_builder
		.use_default_format_selection()
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(config.myWidth, config.myHeight)
		.set_old_swapchain(mySwapchain)
		.build();

	if (!swapchainReturn) 
    {
		Logger::LogError("{} {}", swapchainReturn.error().message(), swapchainReturn.vk_result());
        return false;
	}
    vkDestroySwapchainKHR(myDevice, mySwapchain, nullptr);

    vkb::Swapchain vkbSwapchain = swapchainReturn.value();

	mySwapchain = vkbSwapchain;
    mySwapchainImageFormat = swapchainReturn->image_format;
    mySwapchainImages = vkbSwapchain.get_images().value();
    mySwapchainImageViews = vkbSwapchain.get_image_views().value();
    myWindowExtent.width = config.myWidth;
    myWindowExtent.height = config.myHeight;

    return true;
}

void VulkanBackend::InitCommands()
{
    const VkCommandPoolCreateInfo createInfo = CommandPoolCreateInfo(myGraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_CHECK(vkCreateCommandPool(myDevice, &createInfo, nullptr, &myCommandPool));

    VkCommandBufferAllocateInfo allocInfo = CommandBufferAllocateBuffer(myCommandPool);
    VK_CHECK(vkAllocateCommandBuffers(myDevice, &allocInfo, &myMainCommandBuffer));
}

void VulkanBackend::InitDefaultRenderPass()
{
    VkAttachmentDescription colorAttachmentDesc{};
    colorAttachmentDesc.format = mySwapchainImageFormat;
    colorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDesc{};
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachmentDesc;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDesc;

    VK_CHECK(vkCreateRenderPass(myDevice, &renderPassInfo, nullptr, &myRenderPass));
}

void VulkanBackend::InitFramebuffers(const RendererBackendConfig& config)
{
    VkFramebufferCreateInfo fbInfo{};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.pNext = nullptr;

    fbInfo.renderPass = myRenderPass;
    fbInfo.attachmentCount = 1;
    fbInfo.width = config.myWidth;
    fbInfo.height = config.myHeight;
    fbInfo.layers = 1;

    const u32 swapchainImageCount = static_cast<u32>(mySwapchainImages.size());
    myFramebuffers = Vector<VkFramebuffer>(swapchainImageCount);

    for (u32 i = 0; i < swapchainImageCount; ++i)
    {
        fbInfo.pAttachments = &mySwapchainImageViews[i];
        VK_CHECK(vkCreateFramebuffer(myDevice, &fbInfo, nullptr, &myFramebuffers[i]));
    }
}

void VulkanBackend::Render()
{
    VK_CHECK(vkWaitForFences(myDevice, 1, &myRenderFence, true, 1000000000));
    VK_CHECK(vkResetFences(myDevice, 1, &myRenderFence));

    uint32_t swapchainImageIndex = 0;
    VK_CHECK(vkAcquireNextImageKHR(myDevice, mySwapchain, 1000000000, myPresentSemaphore, nullptr, &swapchainImageIndex));

    VK_CHECK(vkResetCommandBuffer(myMainCommandBuffer, 0));

    VkCommandBuffer cmd = myMainCommandBuffer;

    VkCommandBufferBeginInfo cmdBeginInfo{};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    VkClearValue clearValue{};
    float flash = abs(sin(myFrameNumber / 120.0f));
    clearValue.color = { { 0.0f, 0.0f, flash, 1.0f} };
    VkRenderPassBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass = myRenderPass;
    beginInfo.renderArea.offset.x = 0;
    beginInfo.renderArea.offset.y = 0;
    beginInfo.renderArea.extent = myWindowExtent;
    beginInfo.framebuffer = myFramebuffers[swapchainImageIndex];

    beginInfo.clearValueCount = 1;
    beginInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdEndRenderPass(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit.pWaitDstStageMask = &waitStage;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &myPresentSemaphore;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &myRenderSemaphore;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    VK_CHECK(vkQueueSubmit(myGraphicsQueue, 1, &submit, myRenderFence));

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pSwapchains = &mySwapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.pWaitSemaphores = &myRenderSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices = &swapchainImageIndex;

    VK_CHECK(vkQueuePresentKHR(myGraphicsQueue, &presentInfo));

    myFrameNumber++;
}

VkCommandPoolCreateInfo VulkanBackend::CommandPoolCreateInfo(u32 queueFamilyIndex, VkCommandPoolCreateFlags flags) const
{
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.pNext = nullptr;
	createInfo.queueFamilyIndex = queueFamilyIndex;
    createInfo.flags = flags;

    return createInfo;
}

VkCommandBufferAllocateInfo VulkanBackend::CommandBufferAllocateBuffer(VkCommandPool pool, u32 count, VkCommandBufferLevel level) const
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
	allocInfo.commandPool = pool;
    allocInfo.commandBufferCount = count;
    allocInfo.level = level;
    return allocInfo;
}
