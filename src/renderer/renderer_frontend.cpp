#include "odyssey.h"

using namespace Odyssey;

#include "renderer_frontend.h"
#include "renderer_backend.h"
#include "vulkan/vulkan_backend.h"

RendererFrontend::~RendererFrontend()
{
    delete myBackend;
}

bool RendererFrontend::Initialize()
{
    RendererBackendConfig config{};
    config.myApplicationName = "test";
    myBackend = new VulkanBackend();
    myBackend->Initialize(config);
    return true;    
}