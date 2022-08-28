#pragma once

namespace Odyssey
{
    struct RendererBackendConfig
    {
        const char* myApplicationName{};
    };

    class RendererBackend
    {
    public:
        virtual ~RendererBackend() = default;

        virtual bool Initialize(const RendererBackendConfig& config) = 0;
    };    
}