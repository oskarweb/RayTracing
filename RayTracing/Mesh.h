#pragma once

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <tiny_obj_loader.h>

#include <array>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <iostream>

#include "Vertex.h"
#include "extras.h"

struct Mesh
{
	std::string name = "default";
	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;
	std::unordered_map<Vertex, uint32_t> m_uniqueVertices{};

	VkDevice m_deviceHandle = VK_NULL_HANDLE;
	VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
	VkBuffer m_indexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE;

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
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.string().c_str()))
		{
			return false;
		}

		for (const auto& material : materials)
		{
			std::cout << "MATEIRAL NAEM:                  " << material.metallic_texname << '\n';
			std::cout << "MATEIRAL NAEM:                  " << material.normal_texname << '\n';
			std::cout << "MATEIRAL NAEM:                  " << material.diffuse_texname << '\n';
		}

		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex{};

				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

				vertex.color = { 1.0f, 1.0f, 1.0f };

				if (m_uniqueVertices.count(vertex) == 0) {
					m_uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
					m_vertices.emplace_back(vertex);
				}

				m_indices.emplace_back(m_uniqueVertices[vertex]);
			}
		}
		return true;
	}

	void upload(VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool)
	{
		createVertexBuffer(physicalDevice, graphicsQueue, commandPool);
		createIndexBuffer(physicalDevice, graphicsQueue, commandPool);
	}

	void createIndexBuffer(VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool)
	{
		VkDeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		Helpers::createBuffer(
			m_deviceHandle,
			physicalDevice,
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory
		);

		void* data;
		vkMapMemory(m_deviceHandle, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, m_indices.data(), (size_t)bufferSize);
		vkUnmapMemory(m_deviceHandle, stagingBufferMemory);

		Helpers::createBuffer(
			m_deviceHandle,
			physicalDevice,
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_indexBuffer,
			m_indexBufferMemory
		);

		Helpers::copyBuffer(m_deviceHandle, graphicsQueue, commandPool, stagingBuffer, m_indexBuffer, bufferSize);

		vkDestroyBuffer(m_deviceHandle, stagingBuffer, nullptr);
		vkFreeMemory(m_deviceHandle, stagingBufferMemory, nullptr);
	}

	void createVertexBuffer(VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool)
	{
		VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		Helpers::createBuffer(
			m_deviceHandle,
			physicalDevice,
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory
		);

		void* data;
		vkMapMemory(m_deviceHandle, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, m_vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(m_deviceHandle, stagingBufferMemory);

		Helpers::createBuffer(
			m_deviceHandle,
			physicalDevice,
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_vertexBuffer,
			m_vertexBufferMemory
		);

		Helpers::copyBuffer(m_deviceHandle, graphicsQueue, commandPool, stagingBuffer, m_vertexBuffer, bufferSize);

		vkDestroyBuffer(m_deviceHandle, stagingBuffer, nullptr);
		vkFreeMemory(m_deviceHandle, stagingBufferMemory, nullptr);
	}

	Mesh() = default;
	Mesh(const std::string& name, VkDevice device) : name(name), m_deviceHandle(device) {}

	void cleanup() {
		if (m_indexBuffer != VK_NULL_HANDLE)
			vkDestroyBuffer(m_deviceHandle, m_indexBuffer, nullptr);
		if (m_indexBufferMemory != VK_NULL_HANDLE)
			vkFreeMemory(m_deviceHandle, m_indexBufferMemory, nullptr);
		if (m_vertexBuffer != VK_NULL_HANDLE)
			vkDestroyBuffer(m_deviceHandle, m_vertexBuffer, nullptr);
		if (m_vertexBufferMemory != VK_NULL_HANDLE)
			vkFreeMemory(m_deviceHandle, m_vertexBufferMemory, nullptr);
	}
};