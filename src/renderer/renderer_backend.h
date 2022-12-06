#pragma once

namespace Odyssey
{
    struct RendererBackendConfig
    {
        const char* myApplicationName{};
        int myWidth = 0;
        int myHeight = 0;
    };

    class RendererBackend
    {
    public:
        virtual ~RendererBackend() = default;

        virtual bool Initialize(const RendererBackendConfig& config) = 0;
    };    
}