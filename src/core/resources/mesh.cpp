#include "core/resources/mesh.hpp"
#include "core/vulkan/commands.hpp"
#include <vk_mem_alloc.h>

namespace
{
AllocatedBuffer createBuffer(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage, bool hostVisible)
{
    AllocatedBuffer buffer;
    buffer.allocator = allocator;
    const VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };
    VmaAllocationCreateInfo allocationInfo{};
    allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
    if (hostVisible)
        allocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    if (vmaCreateBuffer(allocator, &bufferInfo, &allocationInfo, &buffer.buffer, &buffer.allocation, nullptr) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to allocate mesh buffer");
    return buffer;
}

AllocatedBuffer uploadBuffer(const vk::raii::Device &device, VmaAllocator allocator, VkQueue queue, VkCommandPool pool,
                             std::span<const std::byte> bytes, VkBufferUsageFlags usage)
{
    auto staging = createBuffer(allocator, bytes.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
    staging.write(bytes);
    auto buffer = createBuffer(allocator, bytes.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, false);
    Helpers::copyBuffer(device, queue, pool, staging.buffer, buffer.buffer, bytes.size());
    return buffer;
}
} // namespace

void Mesh::upload(VmaAllocator allocator, VkQueue graphicsQueue, VkCommandPool commandPool)
{
    if (m_vertices.empty() || m_indices.empty())
        throw std::invalid_argument("Cannot upload an empty mesh");
    if (!m_deviceHandle || !**m_deviceHandle || !allocator || !graphicsQueue || !commandPool)
        throw std::logic_error("Mesh upload requires an initialized device, allocator, queue and command pool");
    auto vertices = uploadBuffer(*m_deviceHandle, allocator, graphicsQueue, commandPool,
                                 std::as_bytes(std::span(m_vertices)), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    auto indices = uploadBuffer(*m_deviceHandle, allocator, graphicsQueue, commandPool,
                                std::as_bytes(std::span(m_indices)), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    // Commit only after both uploads succeed; partial failures release temporaries.
    m_vertexBuffer = std::move(vertices);
    m_indexBuffer = std::move(indices);
}
