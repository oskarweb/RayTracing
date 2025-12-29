#pragma once

#include "extras.hpp"
#include "vertex.hpp"

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
// #include <tiny_obj_loader.h>

#include <array>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <vector>

struct VmaAllocator_T;
using VmaAllocator = VmaAllocator_T *;

struct VmaAllocation_T;
using VmaAllocation = VmaAllocation_T *;

struct Mesh
{
    std::vector<Vertex> m_vertices{};
    std::vector<uint32_t> m_indices{};
    std::unordered_map<Vertex, uint32_t> m_uniqueVertices{};

    VkDevice m_deviceHandle = VK_NULL_HANDLE;
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VmaAllocation m_vertexBufferAlloc = VK_NULL_HANDLE;
    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VmaAllocation m_indexBufferAlloc = VK_NULL_HANDLE;

    Mesh() = default;
    Mesh(VkDevice device) : m_deviceHandle(device) {}

    bool fromVertices(const Vertex *vertices, size_t count)
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

    bool loadFromObj(std::filesystem::path &path)
    {
        // tinyobj::attrib_t attrib;
        // std::vector<tinyobj::shape_t> shapes;
        // std::vector<tinyobj::material_t> materials;
        // std::string warn, err;

        // if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
        // path.string().c_str()))
        // {
        // 	return false;
        // }

        // for (const auto& material : materials)
        // {
        // 	std::cout << "MATEIRAL NAEM:                  " <<
        // material.metallic_texname << '\n'; 	std::cout << "MATEIRAL NAEM: "
        // << material.normal_texname << '\n'; 	std::cout << "MATEIRAL NAEM: "
        // << material.diffuse_texname << '\n';
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
        // 			1.0f - attrib.texcoords[2 * index.texcoord_index
        // + 1]
        // 		};

        // 		vertex.color = { 1.0f, 1.0f, 1.0f };

        // 		if (m_uniqueVertices.count(vertex) == 0) {
        // 			m_uniqueVertices[vertex] =
        // static_cast<uint32_t>(m_vertices.size());
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

    void createIndexBuffer(VmaAllocator &allocator, VkQueue graphicsQueue, VkCommandPool commandPool);
    void createVertexBuffer(VmaAllocator &allocator, VkQueue graphicsQueue, VkCommandPool commandPool);

    void cleanup(VmaAllocator &allocator);
};