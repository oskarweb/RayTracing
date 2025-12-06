#include "mesh.hpp"

#include "vk_mem_alloc.h"

void Mesh::createIndexBuffer(VmaAllocator &allocator, VkQueue graphicsQueue,
                             VkCommandPool commandPool)
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
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocInfo.flags =
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    if (vmaCreateBuffer(allocator, &stagingBufferInfo, &stagingAllocInfo,
                        &stagingBuffer, &stagingAllocation,
                        nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create buffer with VMA!");
    }

    void *data;
    vmaMapMemory(allocator, stagingAllocation, &data);
    memcpy(data, m_indices.data(), (size_t)bufferSize);
    vmaUnmapMemory(allocator, stagingAllocation);

    VkBufferCreateInfo indexBufferInfo{};
    indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    indexBufferInfo.size = bufferSize;
    indexBufferInfo.usage =
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    indexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VmaAllocationCreateInfo indexAllocInfo{};
    indexAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    if (vmaCreateBuffer(allocator, &indexBufferInfo, &indexAllocInfo,
                        &m_indexBuffer, &m_indexBufferAlloc,
                        nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create buffer with VMA!");
    }

    Helpers::copyBuffer(m_deviceHandle, graphicsQueue, commandPool,
                        stagingBuffer, m_indexBuffer, bufferSize);
    vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
}
void Mesh::createVertexBuffer(VmaAllocator &allocator, VkQueue graphicsQueue,
                              VkCommandPool commandPool)
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
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocInfo.flags =
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    if (vmaCreateBuffer(allocator, &stagingBufferInfo, &stagingAllocInfo,
                        &stagingBuffer, &stagingAllocation,
                        nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create buffer with VMA!");
    }

    void *data;
    vmaMapMemory(allocator, stagingAllocation, &data);
    memcpy(data, m_vertices.data(), (size_t)bufferSize);
    vmaUnmapMemory(allocator, stagingAllocation);

    VkBufferCreateInfo vertexBufferInfo{};
    vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexBufferInfo.size = bufferSize;
    vertexBufferInfo.usage =
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VmaAllocationCreateInfo vertexAllocInfo{};
    vertexAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    if (vmaCreateBuffer(allocator, &vertexBufferInfo, &vertexAllocInfo,
                        &m_vertexBuffer, &m_vertexBufferAlloc,
                        nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create buffer with VMA!");
    }

    Helpers::copyBuffer(m_deviceHandle, graphicsQueue, commandPool,
                        stagingBuffer, m_vertexBuffer, bufferSize);
    vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
}
void Mesh::cleanup(VmaAllocator &allocator)
{
    vmaDestroyBuffer(allocator, m_indexBuffer, m_indexBufferAlloc);
    vmaDestroyBuffer(allocator, m_vertexBuffer, m_vertexBufferAlloc);
}