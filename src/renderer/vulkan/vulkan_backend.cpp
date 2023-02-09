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

    vmaDestroyBuffer(myAllocator, mySceneParameterBuffer.myBuffer, mySceneParameterBuffer.myAllocation);
    vmaDestroyBuffer(myAllocator, myMesh.myVertexBuffer.myBuffer, myMesh.myVertexBuffer.myAllocation);

    vkDestroyPipeline(myDevice, myTrianglePipeline, nullptr);
    vkDestroyPipelineLayout(myDevice, myTrianglePipelineLayout, nullptr);

    vkDestroyDescriptorPool(myDevice, myDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(myDevice, myGlobalSetLayout, nullptr);

    for (int i = 0; i < FRAME_OVERLAP; ++i)
    {
        const FrameData& frame = myFrames[i];
        vkDestroySemaphore(myDevice, frame.myPresentSemaphore, nullptr);
        vkDestroySemaphore(myDevice, frame.myRenderSemaphore, nullptr);
        vkDestroyFence(myDevice, frame.myRenderFence, nullptr);
        vkDestroyCommandPool(myDevice, frame.myCommandPool, nullptr);
        vmaDestroyBuffer(myAllocator, frame.myCameraBuffer.myBuffer, frame.myCameraBuffer.myAllocation);
    }

    vmaDestroyImage(myAllocator, myDepthImage.myImage, myDepthImage.myAllocation);
    vkDestroyImageView(myDevice, myDepthImageView, nullptr);

    vkDestroySwapchainKHR(myDevice, mySwapchain, nullptr);
    vkDestroyRenderPass(myDevice, myRenderPass, nullptr);

    for (int i = 0; i < myFramebuffers.size(); ++i)
    {
        vkDestroyFramebuffer(myDevice, myFramebuffers[i], nullptr);
        vkDestroyImageView(myDevice, mySwapchainImageViews[i], nullptr);
    }

    vmaDestroyAllocator(myAllocator);

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

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.device = myDevice;
    allocatorInfo.physicalDevice = myPhysicalDevice;
    allocatorInfo.instance = myInstance;
    vmaCreateAllocator(&allocatorInfo, &myAllocator);

    if (!CreateSwapchain(config))
        return false;

    InitCommands();
    InitDefaultRenderPass();
    InitFramebuffers(config);
    InitSyncStructures();
    InitDescriptors();
    InitPipelines();
    LoadMeshes();

    InitScene();

    myIsInitialized = true;

    return true;
}

void VulkanBackend::InitSyncStructures()
{
    const VkFenceCreateInfo fenceInfo = VulkanInit::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    const VkSemaphoreCreateInfo semaphoreInfo = VulkanInit::SemaphoreCreateInfo();

    for (int i = 0; i < FRAME_OVERLAP; ++i)
    {
        FrameData& frame = myFrames[i];
        VK_CHECK(vkCreateFence(myDevice, &fenceInfo, nullptr, &frame.myRenderFence));
        VK_CHECK(vkCreateSemaphore(myDevice, &semaphoreInfo, nullptr, &frame.myPresentSemaphore));
        VK_CHECK(vkCreateSemaphore(myDevice, &semaphoreInfo, nullptr, &frame.myRenderSemaphore));
    }
}

bool VulkanBackend::CreateInstance()
{
	vkb::InstanceBuilder instanceBuilder;

	constexpr uint32_t logSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT  |
#if VERY_VERBOSE
								 VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT    |
						         VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT       |
#endif
						         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;

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
    myGPUProperties = vkbDevice.physical_device.properties;
    Logger::Log("GPU minimum aligment of {}", myGPUProperties.limits.minUniformBufferOffsetAlignment);

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

    VkExtent3D depthImageExtent = {
        myWindowExtent.width,
        myWindowExtent.height,
        1
    };

    //hardcoding the depth format to 32 bit float
    myDepthFormat = VK_FORMAT_D32_SFLOAT;

    VkImageCreateInfo depthImageInfo = VulkanInit::ImageCreateInfo(myDepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vmaCreateImage(myAllocator, &depthImageInfo, &allocInfo, &myDepthImage.myImage, &myDepthImage.myAllocation, nullptr);

    VkImageViewCreateInfo depthImageViewInfo = VulkanInit::ImageViewCreateInfo(myDepthFormat, myDepthImage.myImage, VK_IMAGE_ASPECT_DEPTH_BIT);

    VK_CHECK(vkCreateImageView(myDevice, &depthImageViewInfo, nullptr, &myDepthImageView));

    return true;
}

AllocatedBuffer VulkanBackend::CreateBuffer(size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) const
{
    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = usage;

    VmaAllocationCreateInfo vmaAllocInfo{};
    vmaAllocInfo.usage = memoryUsage;

    AllocatedBuffer newBuffer{};
    VK_CHECK(vmaCreateBuffer(myAllocator, &info, &vmaAllocInfo, &newBuffer.myBuffer, &newBuffer.myAllocation, nullptr));

    return newBuffer;
}

void VulkanBackend::InitCommands()
{
    const VkCommandPoolCreateInfo createInfo = VulkanInit::CommandPoolCreateInfo(myGraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);


    for (int i = 0; i < FRAME_OVERLAP; ++i)
    {
        VK_CHECK(vkCreateCommandPool(myDevice, &createInfo, nullptr, &myFrames[i].myCommandPool));

        VkCommandBufferAllocateInfo allocInfo = VulkanInit::CommandBufferAllocateBuffer(myFrames[i].myCommandPool);
        VK_CHECK(vkAllocateCommandBuffers(myDevice, &allocInfo, &myFrames[i].myMainCommandBuffer));
    }
}

void VulkanBackend::InitDefaultRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = mySwapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.flags = 0;
    depthAttachment.format = myDepthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDesc{};
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &colorAttachmentRef;
    subpassDesc.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency depthDependency = {};
    depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    depthDependency.dstSubpass = 0;
    depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDependency.srcAccessMask = 0;
    depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };
    VkSubpassDependency dependencies[2] = { dependency, depthDependency };

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDesc;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies;

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
        VkImageView attachments[2];
        attachments[0] = mySwapchainImageViews[i];
        attachments[1] = myDepthImageView;

        fbInfo.attachmentCount = 2;
        fbInfo.pAttachments = attachments;
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

	VkPushConstantRange pushConstant;
	pushConstant.offset = 0;
	pushConstant.size = sizeof(MeshPushConstants);
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	pipelineInfo.pPushConstantRanges = &pushConstant;
	pipelineInfo.pushConstantRangeCount = 1;

    pipelineInfo.setLayoutCount = 1;
    pipelineInfo.pSetLayouts = &myGlobalSetLayout;

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
    pipelineBuilder.myDepthStencil = VulkanInit::DepthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

    myTrianglePipeline = pipelineBuilder.BuildPipeline(myDevice, myRenderPass);

    CreateMaterial(myTrianglePipeline, myTrianglePipelineLayout, "default");

    vkDestroyShaderModule(myDevice, triangleVertexShader, nullptr);
    vkDestroyShaderModule(myDevice, triangleFragShader, nullptr);
}

void VulkanBackend::InitDescriptors()
{
    Vector<VkDescriptorPoolSize> sizes =
    {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 }
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = 0;
    poolInfo.maxSets = 10;
    poolInfo.poolSizeCount = (uint32_t)sizes.size();
    poolInfo.pPoolSizes = sizes.data();

    vkCreateDescriptorPool(myDevice, &poolInfo, nullptr, &myDescriptorPool);

    //binding for camera data at 0
    const VkDescriptorSetLayoutBinding cameraBind = VulkanInit::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
    //binding for scene data at 1
    const VkDescriptorSetLayoutBinding sceneBind = VulkanInit::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);
    const VkDescriptorSetLayoutBinding bindings[] = { cameraBind, sceneBind };

    VkDescriptorSetLayoutCreateInfo setInfo{};
    setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setInfo.bindingCount = 2;
    setInfo.flags = 0;
    setInfo.pBindings = bindings;

    vkCreateDescriptorSetLayout(myDevice, &setInfo, nullptr, &myGlobalSetLayout);

    const size_t sceneParamBufferSize = FRAME_OVERLAP * PadUniformBufferSize(sizeof(GPUSceneData));

    mySceneParameterBuffer = CreateBuffer(sceneParamBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    for (int i = 0; i < FRAME_OVERLAP; ++i)
    {
        FrameData& frame = myFrames[i];
        frame.myCameraBuffer = CreateBuffer(sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = myDescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &myGlobalSetLayout;
        vkAllocateDescriptorSets(myDevice, &allocInfo, &frame.myGlobalDescriptor);

        VkDescriptorBufferInfo cameraInfo{};
        cameraInfo.buffer = frame.myCameraBuffer.myBuffer;
        cameraInfo.offset = 0;
        cameraInfo.range = sizeof(GPUCameraData);

        VkDescriptorBufferInfo sceneInfo;
        sceneInfo.buffer = mySceneParameterBuffer.myBuffer;
        sceneInfo.offset = 0;
        sceneInfo.range = sizeof(GPUSceneData);

        const VkWriteDescriptorSet cameraWrite = VulkanInit::WriteDescriptorBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, frame.myGlobalDescriptor, &cameraInfo, 0);
        const VkWriteDescriptorSet sceneWrite = VulkanInit::WriteDescriptorBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, frame.myGlobalDescriptor, &sceneInfo, 1);
        const VkWriteDescriptorSet setWrites[] = { cameraWrite,sceneWrite };

        vkUpdateDescriptorSets(myDevice, 2, setWrites, 0, nullptr);
    }
}

void VulkanBackend::InitScene()
{
    for (int x = -5; x <= 5; x++)
    {
        for (int y = -5; y <= 5; y++)
        {
            RenderObject tri{};
            tri.myMesh = GetMesh("monke");
            tri.myMaterial = GetMaterial("default");
            tri.myTransformMatrix = glm::translate(glm::mat4{ 1.0 }, glm::vec3(x * 5, 4, y * 5));

            myRenderables.push_back(tri);
        }
    }
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

Material* VulkanBackend::CreateMaterial(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name)
{
    Material mat{};
    mat.myPipeline = pipeline;
    mat.myPipelineLayout = layout;
    myMaterials[name] = mat;
    return &myMaterials[name];
}

Material* VulkanBackend::GetMaterial(const std::string& name)
{
    auto iter = myMaterials.find(name);
    if (iter == myMaterials.end()) {
        return nullptr;
    }
    return &(*iter).second;
}

Mesh* VulkanBackend::GetMesh(const std::string& name)
{
    auto it = myMeshes.find(name);
    if (it == myMeshes.end()) {
        return nullptr;
    }
    return &(*it).second;
}

size_t VulkanBackend::PadUniformBufferSize(size_t originalSize) const
{
    size_t minUboAlignment = myGPUProperties.limits.minUniformBufferOffsetAlignment;
    size_t alignedSize = originalSize;
    if (minUboAlignment > 0)
        alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);

    return alignedSize;
}

void VulkanBackend::DrawObjects(VkCommandBuffer cmd, RenderObject* first, int count)
{
    const glm::vec3 camPos = { cos(PlatformLayer::GetTimeSinceStartup()) * 10,-6.f,sin(PlatformLayer::GetTimeSinceStartup()) * 10};
    const glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);
    glm::mat4 projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.0f);
    projection[1][1] *= -1;

    GPUCameraData camData{};
    camData.myProjection = projection;
    camData.myView = view;
    camData.myViewProjection = projection * view;

    void* data;
    vmaMapMemory(myAllocator, GetCurrentFrame().myCameraBuffer.myAllocation, &data);
    memcpy(data, &camData, sizeof(GPUCameraData));
    vmaUnmapMemory(myAllocator, GetCurrentFrame().myCameraBuffer.myAllocation);

    const float framed = (myFrameNumber / 120.f);
    mySceneParameters.myAmbientColor = { sin(framed),0,cos(framed),1 };
    char* sceneData;
    vmaMapMemory(myAllocator, mySceneParameterBuffer.myAllocation, (void**)&sceneData);
    const int frameIndex = myFrameNumber % FRAME_OVERLAP;
    sceneData += PadUniformBufferSize(sizeof(GPUSceneData)) * frameIndex;
    memcpy(sceneData, &mySceneParameters, sizeof(GPUSceneData));
    vmaUnmapMemory(myAllocator, mySceneParameterBuffer.myAllocation);

    Mesh* lastMesh = nullptr;
    Material* lastMaterial = nullptr;
    for (int i = 0; i < count; i++)
    {
        RenderObject& object = first[i];
        if (object.myMaterial != lastMaterial) {

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.myMaterial->myPipeline);
            lastMaterial = object.myMaterial;

            uint32_t uniformOffset = static_cast<uint32_t>(PadUniformBufferSize(sizeof(GPUSceneData)) * frameIndex);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.myMaterial->myPipelineLayout, 0, 1, &GetCurrentFrame().myGlobalDescriptor, 1, &uniformOffset);
        }

        MeshPushConstants constants;
        constants.myRenderMatrix = object.myTransformMatrix;

        vkCmdPushConstants(cmd, object.myMaterial->myPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

        if (object.myMesh != lastMesh) {
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &object.myMesh->myVertexBuffer.myBuffer, &offset);
            lastMesh = object.myMesh;
        }

        vkCmdDraw(cmd, static_cast<uint32_t>(object.myMesh->myVertices.size()), 1, 0, 0);
    }
}

void VulkanBackend::LoadMeshes()
{
    const std::string binPath = PlatformLayer::GetBinPath();

    myMesh.LoadFromObj(binPath + "/../odyssey/assets_src/meshes/test/monkey_flat.obj");

    UploadMesh(myMesh);

    myMeshes["monke"] = myMesh;
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
    VK_CHECK(vkWaitForFences(myDevice, 1, &GetCurrentFrame().myRenderFence, true, 1000000000));
    VK_CHECK(vkResetFences(myDevice, 1, &GetCurrentFrame().myRenderFence));

    uint32_t swapchainImageIndex = 0;
    VK_CHECK(vkAcquireNextImageKHR(myDevice, mySwapchain, 1000000000, GetCurrentFrame().myPresentSemaphore, nullptr, &swapchainImageIndex));

    VK_CHECK(vkResetCommandBuffer(GetCurrentFrame().myMainCommandBuffer, 0));

    VkCommandBuffer cmd = GetCurrentFrame().myMainCommandBuffer;

    VkCommandBufferBeginInfo cmdBeginInfo{};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    VkClearValue clearColorValue{};
    float flash = abs(sin(myFrameNumber / 120.0f));
    clearColorValue.color = { { 0.0f, 0.0f, flash, 1.0f} };

    VkClearValue clearDepthValue{};
    clearDepthValue.depthStencil.depth = 1.0f;

    VkRenderPassBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass = myRenderPass;
    beginInfo.renderArea.offset.x = 0;
    beginInfo.renderArea.offset.y = 0;
    beginInfo.renderArea.extent = myWindowExtent;
    beginInfo.framebuffer = myFramebuffers[swapchainImageIndex];

    VkClearValue clearValues[] = { clearColorValue, clearDepthValue };

    beginInfo.clearValueCount = 2;
    beginInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

    DrawObjects(cmd, myRenderables.data(), static_cast<uint32_t>(myRenderables.size()));

    vkCmdEndRenderPass(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit.pWaitDstStageMask = &waitStage;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &GetCurrentFrame().myPresentSemaphore;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &GetCurrentFrame().myRenderSemaphore;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    VK_CHECK(vkQueueSubmit(myGraphicsQueue, 1, &submit, GetCurrentFrame().myRenderFence));

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pSwapchains = &mySwapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.pWaitSemaphores = &GetCurrentFrame().myRenderSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices = &swapchainImageIndex;

    VK_CHECK(vkQueuePresentKHR(myGraphicsQueue, &presentInfo));

    myFrameNumber++;
}

FrameData& VulkanBackend::GetCurrentFrame()
{
    return myFrames[myFrameNumber % FRAME_OVERLAP];
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
    pipelineInfo.pDepthStencilState = &myDepthStencil;

    VkPipeline newPipeline{};
    if (vkCreateGraphicsPipelines(
        device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS)
    {
        Logger::LogError("Failed to create pipeline");
        return VK_NULL_HANDLE;
    }

    return newPipeline;
}
