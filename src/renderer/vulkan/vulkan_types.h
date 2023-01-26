#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#define VK_CHECK(expr)                  \
    {                                   \
        ASSERT(expr == VK_SUCCESS);     \
    }

struct AllocatedBuffer
{
    VkBuffer myBuffer{};
    VmaAllocation myAllocation{};
};

struct AllocatedImage
{
    VkImage myImage{};
    VmaAllocation myAllocation{};
};