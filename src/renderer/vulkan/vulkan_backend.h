#pragma once

#include <odyssey/types.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <unordered_map>

#include "renderer/renderer_backend.h"
#include "VkBootstrap.h"
#include "vulkan_mesh.h"

// TODO some of this stuff needs to be moved out

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
    void InitPipelines();

    void InitScene();

    bool LoadShaderModule(const std::string& filePath, VkShaderModule* outShaderModule) const;

    Material* CreateMaterial(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name);
    Material* GetMaterial(const std::string& name);
    Mesh* GetMesh(const std::string& name);

    void DrawObjects(VkCommandBuffer cmd, RenderObject* first, int count) const;

    void LoadMeshes();
    void UploadMesh(Mesh& mesh);

    void Render() override;

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

    VkImageView myDepthImageView{};
    AllocatedImage myDepthImage{};
    VkFormat myDepthFormat{};

    VmaAllocator myAllocator{};

    VkQueue myGraphicsQueue{};
    uint32_t myGraphicsQueueFamily{};

    VkCommandPool myCommandPool{};
    VkCommandBuffer myMainCommandBuffer{};

    VkRenderPass myRenderPass{};
    Vector<VkFramebuffer> myFramebuffers{};

    VkSemaphore myPresentSemaphore{};
    VkSemaphore myRenderSemaphore{};
    VkFence myRenderFence{};

    VkExtent2D myWindowExtent{};


    VkPipelineLayout myTrianglePipelineLayout{};

    VkPipeline myTrianglePipeline{};

    Mesh myMesh{};

    Vector<RenderObject> myRenderables;
    std::unordered_map<std::string, Material> myMaterials;
    std::unordered_map<std::string, Mesh> myMeshes;


    bool myIsInitialized = false;

    // TEMP
    int myFrameNumber = 0;
};

namespace PlatformLayer
{
    VkSurfaceKHR GetVulkanSurface(VkInstance instance);
    bool GetVulkanExtensionNames(std::vector<const char*>& names);
}