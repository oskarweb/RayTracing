#pragma once

#include <cstddef>
#include <span>
#include <utility>
#include <vulkan/vulkan.h>

struct VmaAllocator_T;
using VmaAllocator = VmaAllocator_T *;
struct VmaAllocation_T;
using VmaAllocation = VmaAllocation_T *;

// VMA owns these handles and must destroy both the resource and its allocation.
struct AllocatedBuffer
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
    VmaAllocator allocator = nullptr;

    AllocatedBuffer() = default;
    AllocatedBuffer(const AllocatedBuffer &) = delete;
    AllocatedBuffer &operator=(const AllocatedBuffer &) = delete;
    AllocatedBuffer(AllocatedBuffer &&other) noexcept { swap(other); }
    AllocatedBuffer &operator=(AllocatedBuffer &&other) noexcept
    {
        if (this != &other)
        {
            reset();
            swap(other);
        }
        return *this;
    }
    ~AllocatedBuffer() { reset(); }
    void reset() noexcept;
    void write(std::span<const std::byte> bytes, VkDeviceSize offset = 0) const;
    void swap(AllocatedBuffer &other) noexcept
    {
        std::swap(buffer, other.buffer);
        std::swap(allocation, other.allocation);
        std::swap(allocator, other.allocator);
    }
};

struct Image
{
    VkImage data = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
    VmaAllocator allocator = nullptr;

    Image() = default;
    Image(const Image &) = delete;
    Image &operator=(const Image &) = delete;
    Image(Image &&other) noexcept { swap(other); }
    Image &operator=(Image &&other) noexcept
    {
        if (this != &other)
        {
            reset();
            swap(other);
        }
        return *this;
    }
    ~Image() { reset(); }
    void reset() noexcept;
    void swap(Image &other) noexcept
    {
        std::swap(data, other.data);
        std::swap(allocation, other.allocation);
        std::swap(allocator, other.allocator);
    }
};

class Allocator
{
public:
    VmaAllocator handle = nullptr;
    Allocator() = default;
    Allocator(const Allocator &) = delete;
    Allocator &operator=(const Allocator &) = delete;
    ~Allocator() { reset(); }
    operator VmaAllocator() const noexcept { return handle; }
    void reset() noexcept;
};
