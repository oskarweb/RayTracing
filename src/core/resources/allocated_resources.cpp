#include "core/resources/allocated_resources.hpp"
#include <stdexcept>
#include <vk_mem_alloc.h>

void AllocatedBuffer::write(std::span<const std::byte> bytes, VkDeviceSize offset) const
{
    if (!allocator || !allocation || !buffer)
        throw std::logic_error("Cannot write an unallocated buffer");
    VmaAllocationInfo info{};
    vmaGetAllocationInfo(allocator, allocation, &info);
    if (offset > info.size || bytes.size() > info.size - offset)
        throw std::out_of_range("Buffer write exceeds its allocation");
    if (bytes.empty())
        return;
    // VMA checks mapping and flushes non-coherent memory before returning.
    if (vmaCopyMemoryToAllocation(allocator, bytes.data(), allocation, offset, bytes.size()) != VK_SUCCESS)
        throw std::runtime_error("Failed to write GPU buffer memory");
}

void AllocatedBuffer::reset() noexcept
{
    if (buffer)
        vmaDestroyBuffer(allocator, buffer, allocation);
    buffer = VK_NULL_HANDLE;
    allocation = nullptr;
    allocator = nullptr;
}

void Image::reset() noexcept
{
    if (data)
        vmaDestroyImage(allocator, data, allocation);
    data = VK_NULL_HANDLE;
    allocation = nullptr;
    allocator = nullptr;
}

void Allocator::reset() noexcept
{
    if (handle)
        vmaDestroyAllocator(handle);
    handle = nullptr;
}
