#include "odyssey.h"

#include "odyssey/platform/platform_layer.h"

#include "vulkan_types.h"
#include "vulkan_backend.h"
#include "vulkan_initializers.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <fstream>

// TODO move some of this stuff here to other files.. will be using this for the beginning

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

    vmaDestroyBuffer(myAllocator, myMesh.myVertexBuffer.myBuffer, myMesh.myVertexBuffer.myAllocation);

    vmaDestroyAllocator(myAllocator);

    vkDestroyPipeline(myDevice, myTrianglePipeline, nullptr);
    vkDestroyPipelineLayout(myDevice, myTrianglePipelineLayout, nullptr);

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

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.device = myDevice;
    allocatorInfo.physicalDevice = myPhysicalDevice;
    allocatorInfo.instance = myInstance;
    vmaCreateAllocator(&allocatorInfo, &myAllocator);

    InitCommands();
    InitDefaultRenderPass();
    InitFramebuffers(config);
    InitStructures();
    InitPipelines();

    LoadMeshes();

    myIsInitialized = true;

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

    const vkb::Device vkbDevice = deviceReturn.value();

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
    const VkCommandPoolCreateInfo createInfo = VulkanInit::CommandPoolCreateInfo(myGraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_CHECK(vkCreateCommandPool(myDevice, &createInfo, nullptr, &myCommandPool));

    VkCommandBufferAllocateInfo allocInfo = VulkanInit::CommandBufferAllocateBuffer(myCommandPool);
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

void VulkanBackend::InitPipelines()
{
    PipelineBuilder pipelineBuilder{};

    VertexInputDescription vertexDescription = Vertex::GetVertexInputDescription();
    pipelineBuilder.myVertexInputInfo = VulkanInit::VertexInputStateCreateInfo();

    pipelineBuilder.myVertexInputInfo.pVertexAttributeDescriptions = vertexDescription.myAttributes.data();
    pipelineBuilder.myVertexInputInfo.vertexAttributeDescriptionCount = static_cast<int>(vertexDescription.myAttributes.size());

    pipelineBuilder.myVertexInputInfo.pVertexBindingDescriptions = vertexDescription.myBindings.data();
    pipelineBuilder.myVertexInputInfo.vertexBindingDescriptionCount = static_cast<int>(vertexDescription.myBindings.size());

    pipelineBuilder.myShaderStages.clear();

    const std::string binPath = PlatformLayer::GetBinPath();

    VkShaderModule triangleVertexShader{};
    if (!LoadShaderModule(binPath + "/../odyssey/assets/shaders/triangle.vert.spv", &triangleVertexShader))
    {
        return;
    }

    VkShaderModule triangleFragShader{};
    if (!LoadShaderModule(binPath + "/../odyssey/assets/shaders/triangle.frag.spv", &triangleFragShader))
    {
        return;
    }

    VkPipelineLayoutCreateInfo pipelineInfo = VulkanInit::PipelineLayoutCreateInfo();
    VK_CHECK(vkCreatePipelineLayout(myDevice, &pipelineInfo, nullptr, &myTrianglePipelineLayout));


    pipelineBuilder.myShaderStages.push_back(
        VulkanInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, triangleVertexShader));

    pipelineBuilder.myShaderStages.push_back(
        VulkanInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));


    pipelineBuilder.myInputAssembly = VulkanInit::InputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    pipelineBuilder.myViewport.x = 0.0f;
    pipelineBuilder.myViewport.y = 0.0f;
    pipelineBuilder.myViewport.width = (float)myWindowExtent.width;
    pipelineBuilder.myViewport.height = (float)myWindowExtent.height;
    pipelineBuilder.myViewport.minDepth = 0.0f;
    pipelineBuilder.myViewport.maxDepth = 1.0f;

    pipelineBuilder.myScissor.offset = { 0, 0 };
    pipelineBuilder.myScissor.extent = myWindowExtent;

    pipelineBuilder.myRasterizer = VulkanInit::RasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
    pipelineBuilder.myMultisampling = VulkanInit::MultisamplingStateCreateInfo();
    pipelineBuilder.myColorBlendAttachment = VulkanInit::ColorBlendAttachmentState();
    pipelineBuilder.myPipelineLayout = myTrianglePipelineLayout;

    myTrianglePipeline = pipelineBuilder.BuildPipeline(myDevice, myRenderPass);

    vkDestroyShaderModule(myDevice, triangleVertexShader, nullptr);
    vkDestroyShaderModule(myDevice, triangleFragShader, nullptr);
}

bool VulkanBackend::LoadShaderModule(const std::string& filePath, VkShaderModule* outShaderModule) const
{
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        return false;

    const size_t fileSize = (size_t)file.tellg();
    Vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    file.seekg(0);

    file.read((char*)buffer.data(), fileSize);
    file.close();

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode = buffer.data();

    VkShaderModule shaderModule{};
    if (vkCreateShaderModule(myDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        return false;

    *outShaderModule = shaderModule;
    return true;
}

void VulkanBackend::LoadMeshes()
{
    myMesh.myVertices.resize(3);

    myMesh.myVertices[0].myPosition = { 1.f, 1.f, 0.0f };
    myMesh.myVertices[1].myPosition = { -1.f, 1.f, 0.0f };
    myMesh.myVertices[2].myPosition = { 0.f,-1.f, 0.0f };

    myMesh.myVertices[0].myColor = { 0.f, 1.f, 0.0f };
    myMesh.myVertices[1].myColor = { 0.f, 1.f, 0.0f };
    myMesh.myVertices[2].myColor = { 0.f, 1.f, 0.0f };

    UploadMesh(myMesh);
}

void VulkanBackend::UploadMesh(Mesh& mesh)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = mesh.myVertices.size() * sizeof(Vertex);
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    VK_CHECK(vmaCreateBuffer(myAllocator, &bufferInfo, &allocInfo,
        &mesh.myVertexBuffer.myBuffer,
        &mesh.myVertexBuffer.myAllocation,
        nullptr));

    void* data;
    vmaMapMemory(myAllocator, mesh.myVertexBuffer.myAllocation, &data);

    memcpy(data, mesh.myVertices.data(), mesh.myVertices.size() * sizeof(Vertex));

    vmaUnmapMemory(myAllocator, mesh.myVertexBuffer.myAllocation);
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

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, myTrianglePipeline);

    //bind the mesh vertex buffer with offset 0
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &myMesh.myVertexBuffer.myBuffer, &offset);

    vkCmdDraw(cmd, 3, 1, 0, 0);

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

VkPipeline PipelineBuilder::BuildPipeline(VkDevice device, VkRenderPass pass) const
{
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

    viewportState.viewportCount = 1;
    viewportState.pViewports = &myViewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &myScissor;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &myColorBlendAttachment;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;

    pipelineInfo.stageCount = static_cast<uint32_t>(myShaderStages.size());
    pipelineInfo.pStages = myShaderStages.data();
    pipelineInfo.pVertexInputState = &myVertexInputInfo;
    pipelineInfo.pInputAssemblyState = &myInputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &myRasterizer;
    pipelineInfo.pMultisampleState = &myMultisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = myPipelineLayout;
    pipelineInfo.renderPass = pass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VkPipeline newPipeline{};
    if (vkCreateGraphicsPipelines(
        device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS)
    {
        Logger::LogError("Failed to create pipeline");
        return VK_NULL_HANDLE;
    }

    return newPipeline;
}
