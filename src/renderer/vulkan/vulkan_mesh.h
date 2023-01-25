#pragma once

#include <vector>

#include "odyssey/types.h"
#include "vulkan_types.h"

struct VertexInputDescription
{
	std::vector<VkVertexInputBindingDescription> myBindings{};
	std::vector<VkVertexInputAttributeDescription> myAttributes{};
	VkPipelineVertexInputStateCreateFlags myFlags{};
};

struct Vertex
{
	Vec3 myPosition;
	Vec3 myNormal;
	Vec3 myColor;

	static VertexInputDescription GetVertexInputDescription();
};

struct Mesh
{
	Vector<Vertex> myVertices;
	AllocatedBuffer myVertexBuffer;
};