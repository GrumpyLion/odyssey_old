#pragma once

#include "glm/glm.hpp"

enum class RendererBackend
{
    RENDERER_VULKAN
};

struct GeometryData
{
    glm::mat4 model;
};