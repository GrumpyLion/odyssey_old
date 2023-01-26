#include "renderer/vulkan/vulkan_mesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "odyssey/core/logger.h"

VertexInputDescription Vertex::GetVertexInputDescription()
{
	VertexInputDescription description;

	VkVertexInputBindingDescription mainBinding{};
	mainBinding.binding = 0;
	mainBinding.stride = sizeof(Vertex);
	mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	description.myBindings.push_back(mainBinding);

	VkVertexInputAttributeDescription positionAttribute{};
	positionAttribute.binding = 0;
	positionAttribute.location = 0;
	positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	positionAttribute.offset = offsetof(Vertex, myPosition);

	VkVertexInputAttributeDescription normalAttribute{};
	normalAttribute.binding = 0;
	normalAttribute.location = 1;
	normalAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	normalAttribute.offset = offsetof(Vertex, myNormal);

	VkVertexInputAttributeDescription colorAttribute{};
	colorAttribute.binding = 0;
	colorAttribute.location = 2;
	colorAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	colorAttribute.offset = offsetof(Vertex, myColor);

	description.myAttributes.push_back(positionAttribute);
	description.myAttributes.push_back(normalAttribute);
	description.myAttributes.push_back(colorAttribute);

	return description;
}

bool Mesh::LoadFromObj(const String& filename)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), nullptr);
	if (!warn.empty()) {
		Logger::LogWarn(warn);
	}
	if (!err.empty()) {
		Logger::LogError(err);
		return false;
	}

	for (const auto& shape : shapes) {
		size_t index_offset = 0;
		for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
			const int fv = 3;
			for (size_t v = 0; v < fv; v++) {
				tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];

				tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
				tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
				tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];

				Vertex vertex;
				vertex.myPosition.x = vx;
				vertex.myPosition.y = vy;
				vertex.myPosition.z = vz;

				vertex.myNormal.x = nx;
				vertex.myNormal.y = ny;
				vertex.myNormal.z = nz;

				//we are setting the vertex color as the vertex normal. This is just for display purposes
				vertex.myColor = vertex.myNormal;

				myVertices.push_back(vertex);
			}
			index_offset += fv;
		}
	}

	return true;
}
