#include "odyssey.h"

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
#if USE_VULKAN
	locBackend = new VulkanBackend();
    locBackend->Initialize(config);
#endif
	return true;    
}

bool RendererFrontend::Render()
{
    locBackend->Render();
    return true;
}
