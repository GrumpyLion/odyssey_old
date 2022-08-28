#pragma once

#include "odyssey.h"
#include <vulkan/vulkan.h>

#define VK_CHECK(expr)                  \
    {                                   \
        ASSERT(expr == VK_SUCCESS);     \
    }