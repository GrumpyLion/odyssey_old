#pragma once

#include "glm/glm.hpp"

#include "resources/resource_types.h"

namespace Odyssey
{
    enum class RendererBackend
    {
        RENDERER_VULKAN
    };

    struct GeometryData
    {
        glm::mat4 model;
    };
}