#include "vulkan_renderer.hpp"

#include <stb_image.h>

#include "tiny_obj_loader.h"

#include "vk_mem_alloc.h"

#include "implot.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef NDEBUG
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = true;
#endif

void VulkanRenderer::recreateSwapChain()
{
    m_framebufferWidth = 0;
    m_frameBufferHeight = 0;
    glfwGetFramebufferSize(m_window, &m_framebufferWidth, &m_frameBufferHeight);
    while (m_framebufferWidth == 0 || m_frameBufferHeight == 0)
    {
        glfwWaitEvents();
        glfwGetFramebufferSize(m_window, &m_framebufferWidth, &m_frameBufferHeight);
    }
    vkDeviceWaitIdle(m_device);

    m_prevSwapChain = m_swapChain;

    cleanupSwapChain(false);
    cleanupSyncObjects();

    createSwapChain();
    if (m_prevSwapChain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(m_device, m_prevSwapChain, nullptr);
    createImageViews();
    createColorResources();
    createDepthResources();
    createFramebuffers();
    createSyncObjects();
}

void VulkanRenderer::cleanupSwapChain(bool hard)
{
    vkDestroyImageView(m_device, m_depthImageView, nullptr);
    vmaDestroyImage(m_allocator, m_depthImage.data, m_depthImage.allocation);
    m_depthImage.data = VK_NULL_HANDLE;
    m_depthImage.allocation = {};

    vkDestroyImageView(m_device, m_colorImageView, nullptr);
    vmaDestroyImage(m_allocator, m_colorImage.data, m_colorImage.allocation);
    m_colorImage.data = VK_NULL_HANDLE;
    m_colorImage.allocation = {};

    // for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    // {
    //     vmaDestroyBuffer(m_allocator, m_uniformBuffers[i],
    //     m_uniformBuffersAlloc[i]);
    // }

    for (auto framebuffer : m_swapChainFramebuffers)
    {
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }

    for (auto imageView : m_swapChainImageViews)
    {
        vkDestroyImageView(m_device, imageView, nullptr);
    }

    if (hard)
    {
        vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
    }
}

QueueFamilyIndices VulkanRenderer::findQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    int i = 0;
    for (const auto &queueFamily : queueFamilies)
    {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
        if (presentSupport)
        {
            indices.presentFamily = i;
        }
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }
        if (indices.isComplete())
        {
            break;
        }
        i++;
    }
    return indices;
}

VkFormat VulkanRenderer::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                                             VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

VkFormat VulkanRenderer::findDepthFormat()
{
    return findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                               VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void VulkanRenderer::init()
{
    m_textures.resize(Constants::TEXTURE_COUNT);
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createAllocator();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    PipelineCreateInfo pipelineCreateInfo = {m_device, m_renderPass, m_descriptorSetLayout, m_msaaSamples};
    auto [cubePipeline, cubePipelineLayout] = CubePipeline(m_device, m_physicalDevice).create(pipelineCreateInfo);
    auto [linePipeline, linePipelineLayout] = LinePipeline(m_device, m_physicalDevice).create(pipelineCreateInfo);
    auto [outlinePipeline, outlinePipelineLayout] =
        OutlinePipeline(m_device, m_physicalDevice).create(pipelineCreateInfo);
    m_materials["outline"] = Material("outline", outlinePipeline, outlinePipelineLayout);
    m_materials["cube"] = Material("cube", cubePipeline, cubePipelineLayout);
    m_materials["line"] = Material("line", linePipeline, linePipelineLayout);
    createCommandPool();
    createColorResources();
    createDepthResources();
    createFramebuffers();
    // createTextureSampler();
    m_meshes.emplace("pyramid", Mesh("pyramid", m_device));
    m_meshes["pyramid"].fromVertices(pyramidVertices.data(), pyramidVertices.size());
    m_meshes["pyramid"].upload(m_allocator, m_graphicsQueue, m_commandPool);
    m_meshes.emplace("cube", Mesh("cube", m_device));
    m_meshes["cube"].fromVertices(cubeVertices.data(), cubeVertices.size());
    m_meshes["cube"].upload(m_allocator, m_graphicsQueue, m_commandPool);
    m_meshes.emplace("xAxis", Mesh("xAxis", m_device));
    m_meshes["xAxis"].fromVertices(xAxisVertices.data(), xAxisVertices.size());
    m_meshes["xAxis"].upload(m_allocator, m_graphicsQueue, m_commandPool);
    m_meshes.emplace("yAxis", Mesh("yAxis", m_device));
    m_meshes["yAxis"].fromVertices(yAxisVertices.data(), yAxisVertices.size());
    m_meshes["yAxis"].upload(m_allocator, m_graphicsQueue, m_commandPool);
    m_meshes.emplace("zAxis", Mesh("zAxis", m_device));
    m_meshes["zAxis"].fromVertices(zAxisVertices.data(), zAxisVertices.size());
    m_meshes["zAxis"].upload(m_allocator, m_graphicsQueue, m_commandPool);
    m_meshes.emplace("redline", Mesh("redline", m_device));
    m_meshes["redline"].fromVertices(redLineVertices.data(), redLineVertices.size());
    m_meshes["redline"].upload(m_allocator, m_graphicsQueue, m_commandPool);
    m_meshes.emplace("yellowline", Mesh("yellowline", m_device));
    m_meshes["yellowline"].fromVertices(yellowLineVertices.data(), yellowLineVertices.size());
    m_meshes["yellowline"].upload(m_allocator, m_graphicsQueue, m_commandPool);
    m_meshes.emplace("greenline", Mesh("greenline", m_device));
    m_meshes["greenline"].fromVertices(greenLineVertices.data(), greenLineVertices.size());
    m_meshes["greenline"].upload(m_allocator, m_graphicsQueue, m_commandPool);
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
    initImgui();
}

void VulkanRenderer::initImplVulkanImGui()
{
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = m_instance;
    initInfo.PhysicalDevice = m_physicalDevice;
    initInfo.Device = m_device;
    initInfo.Queue = m_graphicsQueue;
    initInfo.DescriptorPool = m_imguiDescriptorPool;
    initInfo.MinImageCount = static_cast<uint32_t>(m_swapChainImages.size());
    initInfo.ImageCount = static_cast<uint32_t>(m_swapChainImages.size());
    initInfo.RenderPass = m_renderPass;
    initInfo.MSAASamples = m_msaaSamples;
    initInfo.Allocator = nullptr;
    ImGui_ImplVulkan_Init(&initInfo);
}

void VulkanRenderer::initImgui()
{
    m_imguiDrawData.resize(MAX_FRAMES_IN_FLIGHT, nullptr);

    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
    };
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(poolSizes);
    poolInfo.pPoolSizes = poolSizes;

    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_imguiDescriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(m_window, true);
    initImplVulkanImGui();
}

void VulkanRenderer::createAllocator()
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = m_physicalDevice;
    allocatorInfo.device = m_device;
    allocatorInfo.instance = m_instance;

    vmaCreateAllocator(&allocatorInfo, &m_allocator);
}

void VulkanRenderer::createColorResources()
{
    VkFormat colorFormat = m_swapChainImageFormat;

    createImage(m_swapChainExtent.width, m_swapChainExtent.height, 1, m_msaaSamples, colorFormat,
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_colorImage);
    m_colorImageView = createImageView(m_colorImage.data, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void VulkanRenderer::createDepthResources()
{
    VkFormat depthFormat = findDepthFormat();
    createImage(m_swapChainExtent.width, m_swapChainExtent.height, 1, m_msaaSamples, depthFormat,
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthImage);
    m_depthImageView = createImageView(m_depthImage.data, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

VkSampleCountFlagBits VulkanRenderer::getMaxUsableSampleCount()
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
                                physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT)
    {
        return VK_SAMPLE_COUNT_64_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_32_BIT)
    {
        return VK_SAMPLE_COUNT_32_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_16_BIT)
    {
        return VK_SAMPLE_COUNT_16_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_8_BIT)
    {
        return VK_SAMPLE_COUNT_8_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_4_BIT)
    {
        return VK_SAMPLE_COUNT_4_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_2_BIT)
    {
        return VK_SAMPLE_COUNT_2_BIT;
    }

    return VK_SAMPLE_COUNT_1_BIT;
}

void VulkanRenderer::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight,
                                     uint32_t mipLevels)
{
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(m_physicalDevice, imageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    VkCommandBuffer commandBuffer = Helpers::beginSingleTimeCommands(m_device, m_commandPool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &barrier);

        if (mipWidth > 1)
            mipWidth /= 2;
        if (mipHeight > 1)
            mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &barrier);

    Helpers::endSingleTimeCommands(m_device, m_graphicsQueue, m_commandPool, commandBuffer);
}

void VulkanRenderer::createTextureImage(Texture &texture, std::filesystem::path &path)
{
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(path.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels)
    {
        throw std::runtime_error("failed to load texture image!");
    }

    m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;

    VkBufferCreateInfo stagingBufferInfo{};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = imageSize;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VmaAllocationCreateInfo stagingAllocInfo{};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    if (vmaCreateBuffer(m_allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer, &stagingAllocation,
                        nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create buffer with VMA!");
    }

    void *data;
    vmaMapMemory(m_allocator, stagingAllocation, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vmaUnmapMemory(m_allocator, stagingAllocation);

    stbi_image_free(pixels);

    createImage(texWidth, texHeight, m_mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture.image);

    transitionImageLayout(texture.image.data, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mipLevels);
    copyBufferToImage(stagingBuffer, texture.image.data, static_cast<uint32_t>(texWidth),
                      static_cast<uint32_t>(texHeight));
    // transitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB,
    // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_mipLevels);

    vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);

    generateMipmaps(texture.image.data, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, m_mipLevels);
}

void VulkanRenderer::createTextureSampler()
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f; // Optional
    samplerInfo.maxLod = static_cast<float>(m_mipLevels);
    samplerInfo.mipLodBias = 0.0f; // Optional
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void VulkanRenderer::createTextureImageView(VkImageView &textureImageView, VkImage &textureImage)
{
    textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, m_mipLevels);
}

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags,
                                            uint32_t mipLevels)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(m_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image view!");
    }

    return imageView;
}

void VulkanRenderer::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples,
                                 VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                                 VkMemoryPropertyFlags properties, Image &image)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = numSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationInfo allocInfo{};

    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    vmaCreateImage(m_allocator, &imageInfo, &allocCreateInfo, &image.data, &image.allocation, &allocInfo);
}

void VulkanRenderer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout,
                                           VkImageLayout newLayout, uint32_t mipLevels)
{
    VkCommandBuffer commandBuffer = Helpers::beginSingleTimeCommands(m_device, m_commandPool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    Helpers::endSingleTimeCommands(m_device, m_graphicsQueue, m_commandPool, commandBuffer);
}

void VulkanRenderer::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(CameraBuffer);
    m_uniformBuffers.clear();
    m_uniformBuffersMapped.clear();
    m_uniformBuffersAlloc.clear();

    m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    m_uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
    m_uniformBuffersAlloc.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        VmaAllocationInfo vmaInfo{};

        vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &m_uniformBuffers[i], &m_uniformBuffersAlloc[i],
                        &vmaInfo);

        m_uniformBuffersMapped[i] = vmaInfo.pMappedData;
    }
}

void VulkanRenderer::createDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(CameraBuffer);

        std::vector<VkWriteDescriptorSet> descriptorWrites(1);

        if constexpr (Constants::TEXTURE_COUNT > 0)
        {
            descriptorWrites.resize(3);
            std::vector<VkDescriptorImageInfo> imageInfos(Constants::TEXTURE_COUNT);

            for (uint32_t j = 0; j < Constants::TEXTURE_COUNT; ++j)
            {
                imageInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfos[j].imageView = m_textures[j].imageView;
                imageInfos[j].sampler = m_textureSampler;
            }

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = m_descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = imageInfos.data();

            descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[2].dstSet = m_descriptorSets[i];
            descriptorWrites[2].dstBinding = 2;
            descriptorWrites[2].dstArrayElement = 0;
            descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            descriptorWrites[2].descriptorCount = Constants::TEXTURE_COUNT;
            descriptorWrites[2].pImageInfo = imageInfos.data();
        }

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = m_descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0,
                               nullptr);
    }
}

void VulkanRenderer::createDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> poolSizes(1);
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    if constexpr (Constants::TEXTURE_COUNT > 0)
    {
        poolSizes.resize(3);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        poolSizes[2].descriptorCount = static_cast<uint32_t>(Constants::TEXTURE_COUNT * MAX_FRAMES_IN_FLIGHT);
    }
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void VulkanRenderer::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::vector<VkDescriptorSetLayoutBinding> bindings = {uboLayoutBinding};

    if constexpr (Constants::TEXTURE_COUNT > 0)
    {
        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding textureLayoutBinding{};
        textureLayoutBinding.binding = 2;
        textureLayoutBinding.descriptorCount = Constants::TEXTURE_COUNT;
        textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        textureLayoutBinding.pImmutableSamplers = nullptr;
        textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings.insert(bindings.end(), {samplerLayoutBinding, textureLayoutBinding});
    }
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void VulkanRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = Helpers::beginSingleTimeCommands(m_device, m_commandPool);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    Helpers::endSingleTimeCommands(m_device, m_graphicsQueue, m_commandPool, commandBuffer);
}

void VulkanRenderer::createSyncObjects()
{
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(m_swapChainImages.size());
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    m_imagesInFlight = std::vector<VkFence>(m_swapChainImages.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < m_swapChainImages.size(); i++)
    {
        if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create synchronization "
                                     "objects for a frame!");
        }
    }
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create synchronization "
                                     "objects for a frame!");
        }
    }
}

void VulkanRenderer::createCommandBuffers()
{
    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();

    if (vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void VulkanRenderer::drawObjects(VkCommandBuffer &commandBuffer)
{
    Mesh *lastMesh = nullptr;
    Material *lastMaterial = nullptr;
    for (auto &[name, object] : m_renderableObjects)
    {
        if (object.material != lastMaterial)
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 0,
                                    1, &m_descriptorSets[m_currentFrame], 0, nullptr);
            lastMaterial = object.material;
        }

        vkCmdPushConstants(commandBuffer, object.material->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                           sizeof(glm::mat4), &(object.transformMatrix));
        if (object.textureIdx)
        {
            vkCmdPushConstants(commandBuffer, object.material->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
                               sizeof(glm::mat4), sizeof(int), &(object.textureIdx));
        }
        if (object.mesh != lastMesh)
        {
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &object.mesh->m_vertexBuffer, &offset);
            vkCmdBindIndexBuffer(commandBuffer, object.mesh->m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            lastMesh = object.mesh;
        }
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(object.mesh->m_indices.size()), 1, 0, 0, 0);
    }

    // Outline render
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_materials["outline"].pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_materials["outline"].pipelineLayout, 0, 1,
                            &m_descriptorSets[m_currentFrame], 0, nullptr);
    for (auto &[name, object] : m_renderableObjects)
    {
        if (object.mesh->name == "cube" || object.mesh->name == "pyramid")
        {
            vkCmdPushConstants(commandBuffer, m_materials["outline"].pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                               sizeof(glm::mat4), &(object.transformMatrix));
            if (object.mesh != lastMesh)
            {
                VkDeviceSize offset = 0;
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &object.mesh->m_vertexBuffer, &offset);
                vkCmdBindIndexBuffer(commandBuffer, object.mesh->m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                lastMesh = object.mesh;
            }
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(object.mesh->m_indices.size()), 1, 0, 0, 0);
        }
    }
}

void VulkanRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;                  // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.04f, 0.04f, 0.04f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapChainExtent.width);
    viewport.height = static_cast<float>(m_swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    drawObjects(commandBuffer);

    if (m_imguiDrawData[m_currentFrame])
    {
        ImGui_ImplVulkan_RenderDrawData(m_imguiDrawData[m_currentFrame], commandBuffer);
    }

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void VulkanRenderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = Helpers::beginSingleTimeCommands(m_device, m_commandPool);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    Helpers::endSingleTimeCommands(m_device, m_graphicsQueue, m_commandPool, commandBuffer);
}

void VulkanRenderer::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create command pool!");
    }
}

void VulkanRenderer::createRenderPass()
{
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = m_msaaSamples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapChainImageFormat;
    colorAttachment.samples = m_msaaSamples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = m_swapChainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }
}

void VulkanRenderer::createFramebuffers()
{
    m_swapChainFramebuffers.clear();
    m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

    for (size_t i = 0; i < m_swapChainImageViews.size(); i++)
    {
        std::array<VkImageView, 3> attachments = {m_colorImageView, m_depthImageView, m_swapChainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_swapChainExtent.width;
        framebufferInfo.height = m_swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void VulkanRenderer::createImageViews()
{
    m_swapChainImageViews.clear();
    m_swapChainImageViews.resize(m_swapChainImages.size());

    for (uint32_t i = 0; i < m_swapChainImages.size(); i++)
    {
        m_swapChainImageViews[i] =
            createImageView(m_swapChainImages[i], m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

void VulkanRenderer::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;     // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = m_prevSwapChain;

    if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
    m_swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());

    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;
}

void VulkanRenderer::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.wideLines = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(Constants::validationLayers.size());
        createInfo.ppEnabledLayerNames = Constants::validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create logical device!");
    }
    vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
    vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
}

void VulkanRenderer::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    if (deviceCount == 0)
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    for (const auto &device : devices)
    {
        if (isDeviceSuitable(device))
        {
            m_physicalDevice = device;
            m_msaaSamples = getMaxUsableSampleCount();
            break;
        }
    }
    if (m_physicalDevice == VK_NULL_HANDLE)
        throw std::runtime_error("failed to find a suitable GPU!");
}

bool VulkanRenderer::isDeviceSuitable(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy &&
           supportedFeatures.wideLines;
}

bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto &extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

void VulkanRenderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
{
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

void VulkanRenderer::setupDebugMessenger()
{
    if (!enableValidationLayers)
        return;
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);
    if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
        throw std::runtime_error("failed to set up debug messenger!");
}

void VulkanRenderer::createInstance()
{
    if (enableValidationLayers && !checkValidationLayerSupport())
    {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Coulomb Simulation";
    appInfo.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(Constants::validationLayers.size());
        createInfo.ppEnabledLayerNames = Constants::validationLayers.data();
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create instance!");
    }
}

void VulkanRenderer::cleanupSyncObjects()
{
    for (auto semaphore : m_renderFinishedSemaphores)
    {
        vkDestroySemaphore(m_device, semaphore, nullptr);
    }

    for (auto semaphore : m_imageAvailableSemaphores)
    {
        vkDestroySemaphore(m_device, semaphore, nullptr);
    }

    for (auto fence : m_inFlightFences)
    {
        vkDestroyFence(m_device, fence, nullptr);
    }
}

void VulkanRenderer::cleanup()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkWaitForFences(m_device, 1, &m_inFlightFences[i], VK_TRUE, UINT64_MAX);
    }
    vkQueueWaitIdle(m_graphicsQueue);
    vkDeviceWaitIdle(m_device);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    cleanupSwapChain(true);

    for (auto &[name, material] : m_materials)
    {
        if (material.pipeline)
        {
            vkDestroyPipeline(m_device, material.pipeline, nullptr);
        }
        if (material.pipelineLayout)
        {
            vkDestroyPipelineLayout(m_device, material.pipelineLayout, nullptr);
        }
    }

    vkDestroyRenderPass(m_device, m_renderPass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vmaDestroyBuffer(m_allocator, m_uniformBuffers[i], m_uniformBuffersAlloc[i]);
    }

    vkDestroyDescriptorPool(m_device, m_imguiDescriptorPool, nullptr);
    vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

    vkDestroySampler(m_device, m_textureSampler, nullptr);

    for (auto &texture : m_textures)
    {
        if (texture.imageView != VK_NULL_HANDLE)
            vkDestroyImageView(m_device, texture.imageView, nullptr);
        vmaDestroyImage(m_allocator, texture.image.data, texture.image.allocation);
    }

    vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

    for (auto &[name, mesh] : m_meshes)
    {
        mesh.cleanup(m_allocator);
    }

    cleanupSyncObjects();

    vkFreeCommandBuffers(m_device, m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()),
                         m_commandBuffers.data());
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    vmaDestroyAllocator(m_allocator);
    vkDestroyDevice(m_device, nullptr);

    if (enableValidationLayers)
    {
        DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

bool VulkanRenderer::checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName : Constants::validationLayers)
    {
        bool layerFound = false;
        for (const auto &layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }
        if (!layerFound)
            return false;
    }
    return true;
}

std::vector<const char *> VulkanRenderer::getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (enableValidationLayers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRenderer::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                             VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                             const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                             void *pUserData)
{

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

void VulkanRenderer::createSurface()
{
    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface!");
    }
}

SwapChainSupportDetails VulkanRenderer::querySwapChainSupport(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
    }
    return details;
}

VkSurfaceFormatKHR VulkanRenderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
    for (const auto &availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR VulkanRenderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
{
    for (const auto &availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);

        VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

        actualExtent.width =
            std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height =
            std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void VulkanRenderer::newFrame()
{
    vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX,
                                            m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &m_imageAvailableSemaphores[m_currentFrame];
        submitInfo.pWaitDstStageMask = waitStages;

        if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to clean up semaphore!");
        }

        recreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE)
        vkWaitForFences(m_device, 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);

    m_imagesInFlight[imageIndex] = m_inFlightFences[m_currentFrame];

    updateUniformBuffer(m_currentFrame);

    vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);

    vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);
    recordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];
    VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[imageIndex]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = {m_swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized)
    {
        m_framebufferResized = false;
        recreateSwapChain();
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    auto currentFrameTime = std::chrono::high_resolution_clock::now();
    m_deltaTime =
        std::chrono::duration<double, std::chrono::seconds::period>(currentFrameTime - m_lastFrameTime).count();
    m_lastFrameTime = currentFrameTime;

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::updateUniformBuffer(uint32_t currentImage)
{
    CameraBuffer ubo{};

    ubo.view = m_cameraPtr->getViewMatrix();
    ubo.proj =
        glm::perspective(glm::radians(60.0f), m_swapChainExtent.width / (float)m_swapChainExtent.height, 0.1f, 1000.0f);
    // ubo.proj[1][1] *= -1;

    memcpy(m_uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

Material *VulkanRenderer::getMaterial(const std::string &name)
{
    auto it = m_materials.find(name);
    if (it == m_materials.end())
    {
        return nullptr;
    }
    else
    {
        return &(*it).second;
    }
}

Mesh *VulkanRenderer::getMesh(const std::string &name)
{
    auto it = m_meshes.find(name);
    if (it == m_meshes.end())
    {
        return nullptr;
    }
    else
    {
        return &(*it).second;
    }
}

void VulkanRenderer::recordImguiData(ImDrawData *data)
{
    m_imguiDrawData[m_currentFrame] = data;
    return;
}

std::multimap<std::string, Renderable, RenderableComp>::iterator VulkanRenderer::addRenderable(Renderable obj)
{
    obj.id = m_currentRenderableId;
    m_currentRenderableId++;
    return m_renderableObjects.insert(std::pair{(obj.mesh->name + obj.material->name), std::move(obj)});
}

void VulkanRenderer::addRenderables(Model *model)
{
    for (RenderableInfo &info : model->renderableInfos)
    {
        model->renderables.emplace(info.renderableName,
                                   addRenderable(Renderable{getMesh(info.meshName), getMaterial(info.materialName),
                                                            info.transformMatrix, nullptr, 0ull}));
    }
}

void VulkanRenderer::removeRenderable(std::multimap<std::string, Renderable, RenderableComp>::iterator &it)
{
    m_renderableObjects.erase(it);
}