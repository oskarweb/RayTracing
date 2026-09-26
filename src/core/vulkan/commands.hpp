#pragma once

#include "core/vulkan/vulkan_handles.hpp"
#include <cstring>
#include <format>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace Helpers
{
inline void checkVk(VkResult result, std::string_view operation)
{
    if (result != VK_SUCCESS)
        throw std::runtime_error(std::format("{} failed (VkResult {})", operation, static_cast<int>(result)));
}
inline uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1u << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

inline vk::raii::CommandBuffer beginSingleTimeCommands(const vk::raii::Device &device, VkCommandPool commandPool)
{
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;
    auto buffers = device.allocateCommandBuffers(allocInfo);
    auto commandBuffer = std::move(buffers.front());
    commandBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    return commandBuffer;
}

inline void endSingleTimeCommands(VkQueue graphicsQueue, const vk::raii::CommandBuffer &commandBuffer)
{
    commandBuffer.end();
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = rawPtr(commandBuffer);
    checkVk(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE), "Submit upload commands");
    checkVk(vkQueueWaitIdle(graphicsQueue), "Wait for upload completion");
}

inline void copyBuffer(const vk::raii::Device &device, VkQueue graphicsQueue, VkCommandPool commandPool,
                       VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    auto commandBuffer = beginSingleTimeCommands(device, commandPool);
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, 0, size));
    endSingleTimeCommands(graphicsQueue, commandBuffer);
}

inline vk::raii::ShaderModule createShaderModule(const vk::raii::Device &device, std::span<const char> code)
{
    if (code.empty() || code.size() % sizeof(uint32_t) != 0)
        throw std::invalid_argument("SPIR-V must contain a nonempty sequence of 32-bit words");
    std::vector<uint32_t> words(code.size() / sizeof(uint32_t));
    std::memcpy(words.data(), code.data(), code.size());
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = words.data();
    return vk::raii::ShaderModule(device, vk::ShaderModuleCreateInfo(createInfo));
}
} // namespace Helpers
