#include "odyssey.h"

using namespace Odyssey;

#include "renderer_frontend.h"
#include "renderer_backend.h"
#include "vulkan/vulkan_backend.h"

static RendererBackend* locBackend{};

void RendererFrontend::Shutdown()
{
    delete locBackend;
}

bool RendererFrontend::Initialize(int width, int height)
{
    RendererBackendConfig config{};
    config.myApplicationName = "test";
    config.myWidth = width;
    config.myHeight = height;
    locBackend = new VulkanBackend();
    locBackend->Initialize(config);
    return true;    
}