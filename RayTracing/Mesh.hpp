#pragma once

#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"

#include <glm/glm.hpp>
// #include <tiny_obj_loader.h>

#include <array>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <iostream>

#include "Vertex.hpp"
#include "extras.hpp"

struct Mesh
{
	std::string name = "default";
	std::vector<Vertex> m_vertices{};
	std::vector<uint32_t> m_indices{};
	std::unordered_map<Vertex, uint32_t> m_uniqueVertices{};

	VkDevice m_deviceHandle = VK_NULL_HANDLE;
	VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
	VmaAllocation m_vertexBufferAlloc = VK_NULL_HANDLE;
	VkBuffer m_indexBuffer = VK_NULL_HANDLE;
	VmaAllocation m_indexBufferAlloc = VK_NULL_HANDLE;

	bool fromVertices(const Vertex* vertices, size_t count)
	{
		for (uint32_t i = 0; i < count; i++)
		{
			if (m_uniqueVertices.count(vertices[i]) == 0)
			{
				m_uniqueVertices[vertices[i]] = static_cast<uint32_t>(m_vertices.size());
				m_vertices.emplace_back(vertices[i]);
			}
			m_indices.emplace_back(m_uniqueVertices[vertices[i]]);
		}
		return true;
	}

	bool loadFromObj(std::filesystem::path& path)
	{
		// tinyobj::attrib_t attrib;
		// std::vector<tinyobj::shape_t> shapes;
		// std::vector<tinyobj::material_t> materials;
		// std::string warn, err;

		// if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.string().c_str()))
		// {
		// 	return false;
		// }

		// for (const auto& material : materials)
		// {
		// 	std::cout << "MATEIRAL NAEM:                  " << material.metallic_texname << '\n';
		// 	std::cout << "MATEIRAL NAEM:                  " << material.normal_texname << '\n';
		// 	std::cout << "MATEIRAL NAEM:                  " << material.diffuse_texname << '\n';
		// }

		// for (const auto& shape : shapes) {
		// 	for (const auto& index : shape.mesh.indices) {
		// 		Vertex vertex{};

		// 		vertex.pos = {
		// 			attrib.vertices[3 * index.vertex_index + 0],
		// 			attrib.vertices[3 * index.vertex_index + 1],
		// 			attrib.vertices[3 * index.vertex_index + 2]
		// 		};

		// 		vertex.texCoord = {
		// 			attrib.texcoords[2 * index.texcoord_index + 0],
		// 			1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
		// 		};

		// 		vertex.color = { 1.0f, 1.0f, 1.0f };

		// 		if (m_uniqueVertices.count(vertex) == 0) {
		// 			m_uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
		// 			m_vertices.emplace_back(vertex);
		// 		}

		// 		m_indices.emplace_back(m_uniqueVertices[vertex]);
		// 	}
		// }
		return true;
	}

	void upload(VmaAllocator &allocator, VkQueue graphicsQueue, VkCommandPool commandPool)
	{
		createVertexBuffer(allocator, graphicsQueue, commandPool);
		createIndexBuffer(allocator, graphicsQueue, commandPool);
	}

	void createIndexBuffer(VmaAllocator &allocator, VkQueue graphicsQueue, VkCommandPool commandPool)
	{
		VkDeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

    	VkBuffer stagingBuffer;
    	VmaAllocation stagingAllocation;

		VkBufferCreateInfo stagingBufferInfo{};
        stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferInfo.size = bufferSize;
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo stagingAllocInfo{};
        stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        if (vmaCreateBuffer(allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer, &stagingAllocation, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer with VMA!");
        }

    	void* data;
    	vmaMapMemory(allocator, stagingAllocation, &data);
    	memcpy(data, m_indices.data(), (size_t)bufferSize);
    	vmaUnmapMemory(allocator, stagingAllocation);

		VkBufferCreateInfo indexBufferInfo{};
        indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        indexBufferInfo.size = bufferSize;
        indexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        indexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo indexAllocInfo{};
        indexAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        if (vmaCreateBuffer(allocator, &indexBufferInfo, &indexAllocInfo, &m_indexBuffer, &m_indexBufferAlloc, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer with VMA!");
        }

		Helpers::copyBuffer(m_deviceHandle, graphicsQueue, commandPool, stagingBuffer, m_indexBuffer, bufferSize);

    	vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
	}

	void createVertexBuffer(VmaAllocator &allocator, VkQueue graphicsQueue, VkCommandPool commandPool)
	{
		VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

    	VkBuffer stagingBuffer;
    	VmaAllocation stagingAllocation;
		
		VkBufferCreateInfo stagingBufferInfo{};
        stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferInfo.size = bufferSize;
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo stagingAllocInfo{};
        stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;\

        if (vmaCreateBuffer(allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer, &stagingAllocation, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer with VMA!");
        }

		VkBufferCreateInfo vertexBufferInfo{};
        vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertexBufferInfo.size = bufferSize;
        vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo vertexAllocInfo{};
        vertexAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        if (vmaCreateBuffer(allocator, &vertexBufferInfo, &vertexAllocInfo, &m_vertexBuffer, &m_vertexBufferAlloc, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer with VMA!");
        }

		Helpers::copyBuffer(m_deviceHandle, graphicsQueue, commandPool, stagingBuffer, m_vertexBuffer, bufferSize);

    	vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
	}

	Mesh() = default;
	Mesh(const std::string& name, VkDevice device) : name(name), m_deviceHandle(device) {}

	void cleanup(VmaAllocator &allocator) {
		vmaDestroyBuffer(allocator, m_indexBuffer, m_indexBufferAlloc);
		vmaDestroyBuffer(allocator, m_vertexBuffer, m_vertexBufferAlloc);
	}
};