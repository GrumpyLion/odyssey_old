#pragma once

namespace Odyssey
{
    namespace RendererFrontend
    {
        bool Initialize();
        void Shutdown();

        void OnResize(int width, int height);
        bool DrawFrame();
    };    
}