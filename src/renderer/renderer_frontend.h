#pragma once

namespace Odyssey
{
    class RendererBackend;

    class RendererFrontend
    {
    public:
        ~RendererFrontend();

        bool Initialize();
        void OnResize(int width, int height);
        bool DrawFrame();

    private:
        RendererBackend* myBackend{};
    };    
}