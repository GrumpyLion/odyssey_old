#pragma once

namespace Odyssey
{
    namespace RendererFrontend
    {
        bool Initialize(int width, int height);
        void Shutdown();

        void OnResize(int width, int height);
        bool DrawFrame();
    };    
}