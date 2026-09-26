#pragma once

#include "core/resources/allocated_resources.hpp"
#include "core/resources/vertex.hpp"
#include "core/vulkan/vulkan_handles.hpp"

#include <limits>
#include <span>
#include <stdexcept>
#include <unordered_map>
#include <vector>

struct Mesh
{
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    std::unordered_map<Vertex, uint32_t> m_uniqueVertices;
    const vk::raii::Device *m_deviceHandle = nullptr;
    AllocatedBuffer m_vertexBuffer;
    AllocatedBuffer m_indexBuffer;

    Mesh() = default;
    explicit Mesh(const vk::raii::Device &device) noexcept : m_deviceHandle(&device) {}

    void fromVertices(std::span<const Vertex> vertices)
    {
        if (m_vertexBuffer.buffer || m_indexBuffer.buffer)
            throw std::logic_error("Clean up uploaded mesh buffers before replacing vertices");
        if (vertices.size() > std::numeric_limits<uint32_t>::max())
            throw std::length_error("Mesh exceeds the 32-bit index limit");
        Mesh replacement;
        replacement.m_indices.reserve(vertices.size());
        for (const auto &vertex : vertices)
        {
            const auto [it, inserted] =
                replacement.m_uniqueVertices.try_emplace(vertex, static_cast<uint32_t>(replacement.m_vertices.size()));
            if (inserted)
                replacement.m_vertices.push_back(vertex);
            replacement.m_indices.push_back(it->second);
        }
        m_vertices.swap(replacement.m_vertices);
        m_indices.swap(replacement.m_indices);
        m_uniqueVertices.swap(replacement.m_uniqueVertices);
    }

    void upload(VmaAllocator allocator, VkQueue graphicsQueue, VkCommandPool commandPool);
    void cleanup() noexcept
    {
        m_indexBuffer.reset();
        m_vertexBuffer.reset();
    }
};
