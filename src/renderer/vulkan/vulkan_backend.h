#pragma once

#include <odyssey/types.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <unordered_map>

#include "renderer/renderer_backend.h"
#include "VkBootstrap.h"
#include "vulkan_mesh.h"

// TODO some of this stuff needs to be moved out

constexpr uint32_t FRAME_OVERLAP = 2;

struct GPUCameraData
{
    Mat4 myView;
    Mat4 myProjection;
    Mat4 myViewProjection;
};

struct GPUSceneData
{
    Vec4 myFogColor{};
    Vec4 myFogDistances{};
    Vec4 myAmbientColor{};
    Vec4 mySunlightDirection{};
    Vec4 mySunlightColor{};
};

struct MeshPushConstants
{
	Vec4 myData{};
	Mat4 myRenderMatrix{};
};

struct Material
{
    VkPipeline myPipeline{};
    VkPipelineLayout myPipelineLayout{};
};

struct RenderObject
{
    Mesh* myMesh{};
    Material* myMaterial{};
    glm::mat4 myTransformMatrix{};
};

class PipelineBuilder
{
public:
    std::vector<VkPipelineShaderStageCreateInfo> myShaderStages;
    VkPipelineVertexInputStateCreateInfo myVertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo myInputAssembly;
    VkViewport myViewport;
    VkRect2D myScissor;
    VkPipelineRasterizationStateCreateInfo myRasterizer;
    VkPipelineColorBlendAttachmentState myColorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo myMultisampling;
    VkPipelineLayout myPipelineLayout;
    VkPipelineDepthStencilStateCreateInfo myDepthStencil;

    VkPipeline BuildPipeline(VkDevice device, VkRenderPass pass) const;
};

struct FrameData
{
    VkSemaphore myPresentSemaphore, myRenderSemaphore;
    VkFence myRenderFence;

    VkCommandPool myCommandPool;
    VkCommandBuffer myMainCommandBuffer;

    AllocatedBuffer myCameraBuffer;
    VkDescriptorSet myGlobalDescriptor;
};

class VulkanBackend final : public RendererBackend
{
public:
    VulkanBackend();
    ~VulkanBackend() override;

    bool Initialize(const RendererBackendConfig& config) override;
    void InitSyncStructures();

    bool CreateInstance();
    bool CreateDevice();
    bool CreateSwapchain(const RendererBackendConfig& config);
    AllocatedBuffer CreateBuffer(size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) const;

    void InitCommands();
    void InitDefaultRenderPass();
    void InitFramebuffers(const RendererBackendConfig& config);
    void InitPipelines();
    void InitDescriptors();

    void InitScene();

    bool LoadShaderModule(const std::string& filePath, VkShaderModule* outShaderModule) const;

    Material* CreateMaterial(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name);
    Material* GetMaterial(const std::string& name);
    Mesh* GetMesh(const std::string& name);
    size_t PadUniformBufferSize(size_t originalSize) const;

    void DrawObjects(VkCommandBuffer cmd, RenderObject* first, int count);

    void LoadMeshes();
    void UploadMesh(Mesh& mesh);

    void Render() override;

private:
    bool myIsInitialized = false;
    int myFrameNumber = 0;

    vkb::Instance myVKBInstance{};
    VkInstance myInstance{};
    VkSurfaceKHR mySurface{};
    VkDevice myDevice{};
    VkPhysicalDevice myPhysicalDevice{};
    VkPhysicalDeviceProperties myPhysicalDeviceProperties{};
    VkDebugUtilsMessengerEXT myDebugMessenger{};
    VkPhysicalDeviceProperties myGPUProperties{};

    VkSwapchainKHR mySwapchain{};
    VkFormat mySwapchainImageFormat{};
    Vector<VkImage> mySwapchainImages{};
    Vector<VkImageView> mySwapchainImageViews{};

    VkImageView myDepthImageView{};
    AllocatedImage myDepthImage{};
    VkFormat myDepthFormat{};

    VmaAllocator myAllocator{};

    VkQueue myGraphicsQueue{};
    uint32_t myGraphicsQueueFamily{};

    VkRenderPass myRenderPass{};
    Vector<VkFramebuffer> myFramebuffers{};

    VkDescriptorSetLayout myGlobalSetLayout{};
    VkDescriptorPool myDescriptorPool{};

    VkExtent2D myWindowExtent{};

    FrameData myFrames[FRAME_OVERLAP];
    FrameData& GetCurrentFrame();

    GPUSceneData mySceneParameters{};
    AllocatedBuffer mySceneParameterBuffer{};

    Vector<RenderObject> myRenderables;

    // TODO move this?
    VkPipelineLayout myTrianglePipelineLayout{};
    VkPipeline myTrianglePipeline{};
    Mesh myMesh{};

	std::unordered_map<std::string, Material> myMaterials;
    std::unordered_map<std::string, Mesh> myMeshes;
};

namespace PlatformLayer
{
    VkSurfaceKHR GetVulkanSurface(VkInstance instance);
    bool GetVulkanExtensionNames(std::vector<const char*>& names);
}