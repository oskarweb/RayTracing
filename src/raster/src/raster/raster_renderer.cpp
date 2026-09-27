#include "raster/raster_renderer.hpp"
#include "core/vulkan/commands.hpp"
#include "raster/pipeline_factory.hpp"

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
#include <numeric>
#include <optional>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

void VulkanRenderer::recreateSwapChain()
{
    m_framebufferWidth = 0;
    m_framebufferHeight = 0;
    glfwGetFramebufferSize(m_window, &m_framebufferWidth, &m_framebufferHeight);
    while (m_framebufferWidth == 0 || m_framebufferHeight == 0)
    {
        if (glfwWindowShouldClose(m_window))
            return;
        glfwWaitEvents();
        glfwGetFramebufferSize(m_window, &m_framebufferWidth, &m_framebufferHeight);
    }
    Helpers::checkVk(vkDeviceWaitIdle(raw(m_device)), "Wait before swapchain recreation");

    m_prevSwapChain = std::move(m_swapChain);

    cleanupSwapChain(false);
    cleanupSyncObjects();

    createSwapChain();
    m_prevSwapChain.clear();
    createImageViews();
    createColorResources();
    createDepthResources();
    createFramebuffers();
    createSyncObjects();
}

void VulkanRenderer::cleanupSwapChain(bool hard)
{
    m_swapChainFramebuffers.clear();
    m_depthImageView.clear();
    m_depthImage = {};
    m_colorImageView.clear();
    m_colorImage = {};
    m_swapChainImageViews.clear();
    if (hard)
        m_swapChain.clear();
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
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, raw(m_surface), &presentSupport);
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
        vkGetPhysicalDeviceFormatProperties(raw(m_physicalDevice), format, &props);

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
    if (!m_window || !m_cameraPtr)
        throw std::logic_error("Renderer initialization requires a window and camera");
    if (*m_instance)
        throw std::logic_error("Clean up the renderer before initializing it again");
    m_currentFrame = 0;
    m_deltaTime = 0.0;
    m_framebufferResized = false;
    renewModelLifetime();
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
    createMaterials();
    createCommandPool();
    createColorResources();
    createDepthResources();
    createFramebuffers();
    // createTextureSampler();
    std::unordered_map<std::string, const std::vector<Vertex> *> meshesMap = {
        {"pyramid", &pyramidVertices},     {"cube", &cubeVertices},
        {"xAxis", &xAxisVertices},         {"yAxis", &yAxisVertices},
        {"zAxis", &zAxisVertices},         {"redline", &redLineVertices},
        {"greenline", &greenLineVertices}, {"yellowline", &yellowLineVertices}};

    for (auto &[name, vertices] : meshesMap)
    {
        m_meshHandles[name] = m_meshes.insert(Mesh{m_device});
        auto mesh = m_meshes.get(m_meshHandles[name]);
        mesh->fromVertices(*vertices);
        mesh->upload(m_allocator, m_graphicsQueue, raw(m_commandPool));
    }
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createSyncObjects();
    createCommandBuffers();
    initImgui();
    m_lastFrameTime = std::chrono::steady_clock::now();
}

void VulkanRenderer::createMaterials()
{
    GraphicsPipelineConfig cube;
    cube.vertexShader = Constants::SHADERS_PATH / "cube_vert.spv";
    cube.fragmentShader = Constants::SHADERS_PATH / "cube_frag.spv";
    cube.renderPass = *m_renderPass;
    cube.samples = static_cast<vk::SampleCountFlagBits>(m_msaaSamples);
    cube.vertexBindings.emplace_back(Vertex::getBindingDescription());
    for (const auto &attribute : Vertex::getAttributeDescriptions())
        cube.vertexAttributes.emplace_back(attribute);
    vk::PipelineColorBlendAttachmentState alphaBlend;
    alphaBlend.setBlendEnable(true)
        .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
        .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
        .setColorBlendOp(vk::BlendOp::eAdd)
        .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
        .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
        .setAlphaBlendOp(vk::BlendOp::eAdd)
        .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                           vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
    cube.colorAttachments.push_back(alphaBlend);

    auto line = cube;
    line.topology = vk::PrimitiveTopology::eLineList;
    line.lineWidth = m_physicalDevice.getFeatures().wideLines ? 2.0f : 1.0f;

    auto outline = cube;
    outline.vertexShader = Constants::SHADERS_PATH / "outline_vert.spv";
    outline.fragmentShader = Constants::SHADERS_PATH / "outline_frag.spv";
    outline.cullMode = vk::CullModeFlagBits::eFront;
    outline.depthWrite = false;
    outline.depthCompare = vk::CompareOp::eLessOrEqual;

    auto paraboloid = cube;
    paraboloid.cullMode = vk::CullModeFlagBits::eNone;

    const std::array pushConstants = {
        vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4)),
        vk::PushConstantRange(vk::ShaderStageFlagBits::eFragment, sizeof(glm::mat4), sizeof(int))};
    vk::PipelineLayoutCreateInfo layoutInfo;
    layoutInfo.setSetLayouts(*m_descriptorSetLayout).setPushConstantRanges(pushConstants);
    const auto addMaterial = [&](const std::string &name, const GraphicsPipelineConfig &config) {
        vk::raii::PipelineLayout layout(m_device, layoutInfo);
        auto pipeline = createGraphicsPipeline(m_device, *layout, config);
        m_materialHandles[name] = m_materials.insert(Material{std::move(pipeline), std::move(layout)});
    };
    addMaterial("outline", outline);
    addMaterial("cube", cube);
    addMaterial("line", line);
    addMaterial("paraboloid", paraboloid);
}

void VulkanRenderer::initImplVulkanImGui()
{
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = raw(m_instance);
    initInfo.PhysicalDevice = raw(m_physicalDevice);
    initInfo.Device = raw(m_device);
    initInfo.Queue = m_graphicsQueue;
    initInfo.DescriptorPool = raw(m_imguiDescriptorPool);
    initInfo.MinImageCount = static_cast<uint32_t>(m_swapChainImages.size());
    initInfo.ImageCount = static_cast<uint32_t>(m_swapChainImages.size());
    initInfo.PipelineInfoMain.RenderPass = raw(m_renderPass);
    initInfo.PipelineInfoMain.MSAASamples = m_msaaSamples;
    initInfo.Allocator = nullptr;
    ImGui_ImplVulkan_Init(&initInfo);
}

void VulkanRenderer::initImgui()
{
    m_imguiDrawData.assign(MAX_FRAMES_IN_FLIGHT, nullptr);

    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
    };
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(poolSizes);
    poolInfo.pPoolSizes = poolSizes;

    m_imguiDescriptorPool = vk::raii::DescriptorPool(m_device, vk::DescriptorPoolCreateInfo(poolInfo));
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(m_window, true);
    initImplVulkanImGui();
    m_imguiInitialized = true;
}

void VulkanRenderer::createAllocator()
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = raw(m_physicalDevice);
    allocatorInfo.device = raw(m_device);
    allocatorInfo.instance = raw(m_instance);

    if (vmaCreateAllocator(&allocatorInfo, &m_allocator.handle) != VK_SUCCESS)
        throw std::runtime_error("failed to create VMA allocator!");
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
    vkGetPhysicalDeviceProperties(raw(m_physicalDevice), &physicalDeviceProperties);

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
    vkGetPhysicalDeviceFormatProperties(raw(m_physicalDevice), imageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    auto commandOwner = Helpers::beginSingleTimeCommands(m_device, raw(m_commandPool));
    VkCommandBuffer commandBuffer = raw(commandOwner);

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

    Helpers::endSingleTimeCommands(m_graphicsQueue, commandOwner);
}

void VulkanRenderer::createTextureImage(Texture &texture, const std::filesystem::path &path)
{
    int texWidth = 0, texHeight = 0, texChannels = 0;
    const std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels(
        stbi_load(path.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha), stbi_image_free);

    if (!pixels || texWidth <= 0 || texHeight <= 0)
    {
        throw std::runtime_error("failed to load texture image!");
    }

    const VkDeviceSize imageSize =
        static_cast<VkDeviceSize>(texWidth) * static_cast<VkDeviceSize>(texHeight) * STBI_rgb_alpha;
    m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    AllocatedBuffer staging;
    staging.allocator = m_allocator;

    VkBufferCreateInfo stagingBufferInfo{};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = imageSize;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VmaAllocationCreateInfo stagingAllocInfo{};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    if (vmaCreateBuffer(m_allocator, &stagingBufferInfo, &stagingAllocInfo, &staging.buffer, &staging.allocation,
                        nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create buffer with VMA!");
    }

    staging.write(std::as_bytes(std::span(pixels.get(), static_cast<size_t>(imageSize))));

    createImage(texWidth, texHeight, m_mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture.image);

    transitionImageLayout(texture.image.data, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mipLevels);
    copyBufferToImage(staging.buffer, texture.image.data, static_cast<uint32_t>(texWidth),
                      static_cast<uint32_t>(texHeight));
    // transitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB,
    // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_mipLevels);

    staging.reset();

    generateMipmaps(texture.image.data, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, m_mipLevels);
}

void VulkanRenderer::createTextureSampler()
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(raw(m_physicalDevice), &properties);

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

    m_textureSampler = vk::raii::Sampler(m_device, vk::SamplerCreateInfo(samplerInfo));
}

void VulkanRenderer::createTextureImageView(vk::raii::ImageView &textureImageView, VkImage &textureImage)
{
    textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, m_mipLevels);
}

vk::raii::ImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags,
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

    vk::raii::ImageView imageView{nullptr};
    imageView = vk::raii::ImageView(m_device, vk::ImageViewCreateInfo(viewInfo));

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

    image.allocator = m_allocator;
    if (vmaCreateImage(m_allocator, &imageInfo, &allocCreateInfo, &image.data, &image.allocation, &allocInfo) !=
        VK_SUCCESS)
        throw std::runtime_error("failed to create image with VMA!");
}

void VulkanRenderer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout,
                                           VkImageLayout newLayout, uint32_t mipLevels)
{
    auto commandOwner = Helpers::beginSingleTimeCommands(m_device, raw(m_commandPool));
    VkCommandBuffer commandBuffer = raw(commandOwner);

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

    Helpers::endSingleTimeCommands(m_graphicsQueue, commandOwner);
}

void VulkanRenderer::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(CameraBuffer);
    m_uniformBuffers.clear();

    m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        VmaAllocationInfo vmaInfo{};

        m_uniformBuffers[i].allocator = m_allocator;
        if (vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &m_uniformBuffers[i].buffer,
                            &m_uniformBuffers[i].allocation, &vmaInfo) != VK_SUCCESS)
            throw std::runtime_error("failed to create uniform buffer!");
    }
}

void VulkanRenderer::createDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, raw(m_descriptorSetLayout));
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = raw(m_descriptorPool);
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    m_descriptorSets.reserve(MAX_FRAMES_IN_FLIGHT);
    m_descriptorSets = vk::raii::DescriptorSets(m_device, vk::DescriptorSetAllocateInfo(allocInfo));

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[i].buffer;
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
                imageInfos[j].imageView = raw(m_textures[j].imageView);
                imageInfos[j].sampler = raw(m_textureSampler);
            }

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = raw(m_descriptorSets[i]);
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = imageInfos.data();

            descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[2].dstSet = raw(m_descriptorSets[i]);
            descriptorWrites[2].dstBinding = 2;
            descriptorWrites[2].dstArrayElement = 0;
            descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            descriptorWrites[2].descriptorCount = Constants::TEXTURE_COUNT;
            descriptorWrites[2].pImageInfo = imageInfos.data();
        }

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = raw(m_descriptorSets[i]);
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(raw(m_device), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(),
                               0, nullptr);
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
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    m_descriptorPool = vk::raii::DescriptorPool(m_device, vk::DescriptorPoolCreateInfo(poolInfo));
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

    m_descriptorSetLayout = vk::raii::DescriptorSetLayout(m_device, vk::DescriptorSetLayoutCreateInfo(layoutInfo));
}

void VulkanRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    auto commandOwner = Helpers::beginSingleTimeCommands(m_device, raw(m_commandPool));
    VkCommandBuffer commandBuffer = raw(commandOwner);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    Helpers::endSingleTimeCommands(m_graphicsQueue, commandOwner);
}

void VulkanRenderer::createSyncObjects()
{
    m_imageAvailableSemaphores.clear();
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        m_imageAvailableSemaphores.emplace_back(nullptr);
    m_renderFinishedSemaphores.clear();
    for (size_t i = 0; i < m_swapChainImages.size(); ++i)
        m_renderFinishedSemaphores.emplace_back(nullptr);
    m_inFlightFences.clear();
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        m_inFlightFences.emplace_back(nullptr);
    m_imagesInFlight = std::vector<VkFence>(m_swapChainImages.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < m_swapChainImages.size(); i++)
    {
        m_renderFinishedSemaphores[i] = vk::raii::Semaphore(m_device, vk::SemaphoreCreateInfo(semaphoreInfo));
    }
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_inFlightFences[i] = vk::raii::Fence(m_device, vk::FenceCreateInfo(fenceInfo));
        m_imageAvailableSemaphores[i] = vk::raii::Semaphore(m_device, vk::SemaphoreCreateInfo(semaphoreInfo));
    }
}

void VulkanRenderer::createCommandBuffers()
{
    m_commandBuffers.reserve(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = raw(m_commandPool);
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    m_commandBuffers = vk::raii::CommandBuffers(m_device, vk::CommandBufferAllocateInfo(allocInfo));
}

void VulkanRenderer::drawObjects(VkCommandBuffer &commandBuffer)
{
    uint32_t lastMeshIdx = UINT32_MAX;
    uint32_t lastMaterialIdx = UINT32_MAX;

    std::vector<uint32_t> drawOrder(m_renderObjects.dense().size());
    std::iota(drawOrder.begin(), drawOrder.end(), 0);
    auto &renderObjectDense = m_renderObjects.dense();

    std::sort(drawOrder.begin(), drawOrder.end(), [&](uint32_t ia, uint32_t ib) {
        const auto hMatA = renderObjectDense[ia].value.hMaterial;
        const auto hMatB = renderObjectDense[ib].value.hMaterial;

        if (hMatA.index != hMatB.index)
            return hMatA.index < hMatB.index;

        const auto hMeshA = renderObjectDense[ia].value.hMesh;
        const auto hMeshB = renderObjectDense[ib].value.hMesh;

        return hMeshA.index < hMeshB.index;
    });

    for (const auto idx : drawOrder)
    {
        auto &ro = renderObjectDense[idx].value;
        Material *material = m_materials.get(ro.hMaterial);
        Mesh *mesh = m_meshes.get(ro.hMesh);
        if (!material || !mesh)
            continue;

        if (ro.hMaterial.index != lastMaterialIdx)
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, raw(material->pipeline));
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, raw(material->pipelineLayout), 0, 1,
                                    rawPtr(m_descriptorSets[m_currentFrame]), 0, nullptr);
            lastMaterialIdx = ro.hMaterial.index;
        }

        vkCmdPushConstants(commandBuffer, raw(material->pipelineLayout), VK_SHADER_STAGE_VERTEX_BIT, 0,
                           sizeof(glm::mat4), &(ro.transformMatrix));
        if (ro.textureIdx != -1)
        {
            vkCmdPushConstants(commandBuffer, raw(material->pipelineLayout), VK_SHADER_STAGE_FRAGMENT_BIT,
                               sizeof(glm::mat4), sizeof(int), &(ro.textureIdx));
        }
        if (ro.hMesh.index != lastMeshIdx)
        {
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &mesh->m_vertexBuffer.buffer, &offset);
            vkCmdBindIndexBuffer(commandBuffer, mesh->m_indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
            lastMeshIdx = ro.hMesh.index;
        }
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh->m_indices.size()), 1, 0, 0, 0);
    }

    // Outline render
    Material *outlineMat = m_materials.get(m_materialHandles.at("outline"));
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, raw(outlineMat->pipeline));
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, raw(outlineMat->pipelineLayout), 0, 1,
                            rawPtr(m_descriptorSets[m_currentFrame]), 0, nullptr);

    for (const auto idx : drawOrder)
    {
        auto &ro = renderObjectDense[idx].value;
        Material *material = m_materials.get(ro.hMaterial);
        Mesh *mesh = m_meshes.get(ro.hMesh);
        if (!material || !mesh)
            continue;

        if (ro.hMesh.index == m_meshHandles.at("cube").index || ro.hMesh.index == m_meshHandles.at("pyramid").index)
        {
            vkCmdPushConstants(commandBuffer, raw(outlineMat->pipelineLayout), VK_SHADER_STAGE_VERTEX_BIT, 0,
                               sizeof(glm::mat4), &(ro.transformMatrix));
            if (ro.hMesh.index != lastMeshIdx)
            {
                VkDeviceSize offset = 0;
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &mesh->m_vertexBuffer.buffer, &offset);
                vkCmdBindIndexBuffer(commandBuffer, mesh->m_indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
                lastMeshIdx = ro.hMesh.index;
            }
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh->m_indices.size()), 1, 0, 0, 0);
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
    renderPassInfo.renderPass = raw(m_renderPass);
    renderPassInfo.framebuffer = raw(m_swapChainFramebuffers[imageIndex]);
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
    auto commandOwner = Helpers::beginSingleTimeCommands(m_device, raw(m_commandPool));
    VkCommandBuffer commandBuffer = raw(commandOwner);

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

    Helpers::endSingleTimeCommands(m_graphicsQueue, commandOwner);
}

void VulkanRenderer::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(raw(m_physicalDevice));

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    m_commandPool = vk::raii::CommandPool(m_device, vk::CommandPoolCreateInfo(poolInfo));
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

    m_renderPass = vk::raii::RenderPass(m_device, vk::RenderPassCreateInfo(renderPassInfo));
}

void VulkanRenderer::createFramebuffers()
{
    m_swapChainFramebuffers.clear();
    m_swapChainFramebuffers.clear();
    for (size_t i = 0; i < m_swapChainImageViews.size(); ++i)
        m_swapChainFramebuffers.emplace_back(nullptr);

    for (size_t i = 0; i < m_swapChainImageViews.size(); i++)
    {
        std::array<VkImageView, 3> attachments = {raw(m_colorImageView), raw(m_depthImageView),
                                                  raw(m_swapChainImageViews[i])};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = raw(m_renderPass);
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_swapChainExtent.width;
        framebufferInfo.height = m_swapChainExtent.height;
        framebufferInfo.layers = 1;

        m_swapChainFramebuffers[i] = vk::raii::Framebuffer(m_device, vk::FramebufferCreateInfo(framebufferInfo));
    }
}

void VulkanRenderer::createImageViews()
{
    m_swapChainImageViews.clear();
    m_swapChainImageViews.clear();
    for (size_t i = 0; i < m_swapChainImages.size(); ++i)
        m_swapChainImageViews.emplace_back(nullptr);

    for (uint32_t i = 0; i < m_swapChainImages.size(); i++)
    {
        m_swapChainImageViews[i] =
            createImageView(m_swapChainImages[i], m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

void VulkanRenderer::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(raw(m_physicalDevice));

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
    createInfo.surface = raw(m_surface);
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(raw(m_physicalDevice));
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
    createInfo.oldSwapchain = raw(m_prevSwapChain);

    m_swapChain = vk::raii::SwapchainKHR(m_device, vk::SwapchainCreateInfoKHR(createInfo));

    vkGetSwapchainImagesKHR(raw(m_device), raw(m_swapChain), &imageCount, nullptr);
    m_swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(raw(m_device), raw(m_swapChain), &imageCount, m_swapChainImages.data());

    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;
}

void VulkanRenderer::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(raw(m_physicalDevice));
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

    m_device = vk::raii::Device(m_physicalDevice, vk::DeviceCreateInfo(createInfo));
    vkGetDeviceQueue(raw(m_device), indices.presentFamily.value(), 0, &m_presentQueue);
    vkGetDeviceQueue(raw(m_device), indices.graphicsFamily.value(), 0, &m_graphicsQueue);
}

void VulkanRenderer::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(raw(m_instance), &deviceCount, nullptr);
    if (deviceCount == 0)
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(raw(m_instance), &deviceCount, devices.data());

    for (const auto &device : devices)
    {
        if (isDeviceSuitable(device))
        {
            m_physicalDevice = vk::raii::PhysicalDevice(m_instance, device);
            m_msaaSamples = getMaxUsableSampleCount();
            break;
        }
    }
    if (raw(m_physicalDevice) == VK_NULL_HANDLE)
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
    m_debugMessenger = vk::raii::DebugUtilsMessengerEXT(m_instance, vk::DebugUtilsMessengerCreateInfoEXT(createInfo));
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

    m_instance = vk::raii::Instance(m_context, vk::InstanceCreateInfo(createInfo));
}

void VulkanRenderer::cleanupSyncObjects()
{
    m_renderFinishedSemaphores.clear();
    m_imageAvailableSemaphores.clear();
    m_inFlightFences.clear();
    m_imagesInFlight.clear();
}

void VulkanRenderer::cleanup()
{
    invalidateModels();
    m_renderObjects = {};
    m_imguiDrawData.clear();
    m_currentFrame = 0;
    m_deltaTime = 0.0;
    if (!*m_device)
    {
        m_surface.clear();
        m_debugMessenger.clear();
        m_physicalDevice.clear();
        m_instance.clear();
        return;
    }
    m_device.waitIdle();
    if (m_imguiInitialized)
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
        m_imguiInitialized = false;
    }
    m_commandBuffers.clear();
    cleanupSwapChain(true);
    m_prevSwapChain.clear();
    m_materials = {};
    m_materialHandles.clear();
    m_renderPass.clear();
    m_descriptorSets.clear();
    m_imguiDescriptorPool.clear();
    m_descriptorPool.clear();
    m_uniformBuffers.clear();
    m_textureSampler.clear();
    for (auto &texture : m_textures)
    {
        texture.imageView.clear();
        texture.image.reset();
    }
    m_textures.clear();
    m_descriptorSetLayout.clear();
    for (auto &mesh : m_meshes.dense())
        mesh.value.cleanup();
    m_meshes = {};
    m_meshHandles.clear();
    cleanupSyncObjects();
    m_commandPool.clear();
    m_allocator.reset();
    m_device.clear();
    m_surface.clear();
    m_debugMessenger.clear();
    m_physicalDevice.clear();
    m_instance.clear();
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
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (glfwCreateWindowSurface(raw(m_instance), m_window, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface!");
    }
    m_surface = vk::raii::SurfaceKHR(m_instance, surface);
}

SwapChainSupportDetails VulkanRenderer::querySwapChainSupport(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, raw(m_surface), &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, raw(m_surface), &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, raw(m_surface), &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, raw(m_surface), &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, raw(m_surface), &presentModeCount,
                                                  details.presentModes.data());
    }
    return details;
}

VkSurfaceFormatKHR VulkanRenderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
    if (availableFormats.empty())
        throw std::runtime_error("No swapchain surface formats available");
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
    if (!m_imguiInitialized)
        throw std::logic_error("Cannot draw before renderer initialization");
    if (glfwWindowShouldClose(m_window))
        return;
    Helpers::checkVk(vkWaitForFences(raw(m_device), 1, rawPtr(m_inFlightFences[m_currentFrame]), VK_TRUE, UINT64_MAX),
                     "Wait for frame fence");
    uint32_t imageIndex;
    VkResult result =
        vkAcquireNextImageKHR(raw(m_device), raw(m_swapChain), UINT64_MAX,
                              raw(m_imageAvailableSemaphores[m_currentFrame]), VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // No image was acquired, so the acquisition semaphore was not signaled.
        recreateSwapChain();
        return;
    }
    const bool suboptimal = result == VK_SUBOPTIMAL_KHR;
    if (result != VK_SUCCESS && !suboptimal)
        Helpers::checkVk(result, "Acquire swapchain image");

    if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE)
        Helpers::checkVk(vkWaitForFences(raw(m_device), 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX),
                         "Wait for swapchain image fence");

    m_imagesInFlight[imageIndex] = raw(m_inFlightFences[m_currentFrame]);

    updateUniformBuffer(m_currentFrame);

    Helpers::checkVk(vkResetCommandBuffer(raw(m_commandBuffers[m_currentFrame]), 0), "Reset frame command buffer");
    recordCommandBuffer(raw(m_commandBuffers[m_currentFrame]), imageIndex);
    Helpers::checkVk(vkResetFences(raw(m_device), 1, rawPtr(m_inFlightFences[m_currentFrame])), "Reset frame fence");
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {raw(m_imageAvailableSemaphores[m_currentFrame])};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = rawPtr(m_commandBuffers[m_currentFrame]);
    VkSemaphore signalSemaphores[] = {raw(m_renderFinishedSemaphores[imageIndex])};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, raw(m_inFlightFences[m_currentFrame])) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = {raw(m_swapChain)};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
    if (result != VK_SUCCESS && result != VK_ERROR_OUT_OF_DATE_KHR && result != VK_SUBOPTIMAL_KHR)
        Helpers::checkVk(result, "Present swapchain image");
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || suboptimal || m_framebufferResized)
    {
        m_framebufferResized = false;
        recreateSwapChain();
    }

    auto currentFrameTime = std::chrono::steady_clock::now();
    m_deltaTime = std::chrono::duration<double>(currentFrameTime - m_lastFrameTime).count();
    m_lastFrameTime = currentFrameTime;

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::updateUniformBuffer(uint32_t currentImage)
{
    m_cameraUBO.view = m_cameraPtr->getViewMatrix();
    m_cameraUBO.proj = glm::perspective(
        glm::radians(60.0f), static_cast<float>(m_swapChainExtent.width) / static_cast<float>(m_swapChainExtent.height),
        0.1f, 1000.0f);
    // ubo.proj[1][1] *= -1;

    m_uniformBuffers[currentImage].write(std::as_bytes(std::span(&m_cameraUBO, 1)));
}

Ray VulkanRenderer::castRayIntoWorld(float screenX, float screenY)
{
    // GLFW cursor coordinates are in window units, not framebuffer pixels.
    int width = 0, height = 0;
    if (m_window)
        glfwGetWindowSize(m_window, &width, &height);
    if (width <= 0 || height <= 0 || m_framebufferWidth <= 0 || m_framebufferHeight <= 0)
        return {glm::vec3(0.0f), glm::vec3(0.0f)};
    float x = (2.0f * screenX / width) - 1.0f;
    float y = (2.0f * screenY / height) - 1.0f;

    glm::mat4 invVP = glm::inverse(m_cameraUBO.proj * m_cameraUBO.view);

    glm::vec4 nearP = invVP * glm::vec4(x, y, 0.0f, 1.0f);
    glm::vec4 farP = invVP * glm::vec4(x, y, 1.0f, 1.0f);

    nearP /= nearP.w;
    farP /= farP.w;

    glm::vec3 origin = glm::vec3(nearP);
    glm::vec3 dir = glm::normalize(glm::vec3(farP - nearP));

    return {origin, dir};
}

SparseSet<Material>::Handle VulkanRenderer::getMaterial(const std::string &name)
{
    auto it = m_materialHandles.find(name);
    if (it == m_materialHandles.end())
        throw std::out_of_range("Unknown material: " + name);
    return it->second;
}

SparseSet<Mesh>::Handle VulkanRenderer::getMesh(const std::string &name)
{
    auto it = m_meshHandles.find(name);
    if (it == m_meshHandles.end())
        throw std::out_of_range("Unknown mesh: " + name);
    return it->second;
}

void VulkanRenderer::recordImguiData(ImDrawData *data) { m_imguiDrawData.at(m_currentFrame) = data; }

SparseSet<RenderObject>::Handle VulkanRenderer::addRenderObject(RenderObject obj)
{
    if (!m_meshes.get(obj.hMesh) || !m_materials.get(obj.hMaterial))
        throw std::invalid_argument("Render object references a missing mesh or material");
    return m_renderObjects.insert(obj);
}

void VulkanRenderer::removeRenderObject(SparseSet<RenderObject>::Handle handle) { m_renderObjects.remove(handle); }

std::string VulkanRenderer::createParaboloid(std::string meshName, int nx, int nz,
                                             const std::function<double(double, double)> &formula)
{
    const std::string baseName = meshName;
    size_t suffix = 1;
    while (m_meshHandles.contains(meshName))
        meshName = baseName + std::to_string(suffix++);
    auto paraboloidVertices =
        generateParaboloidVertices(nx, nz, -1.0, 1.0, -1.0, 1.0, glm::vec3{1.0, 0.0, 0.0}, formula);
    Mesh mesh(m_device);
    mesh.fromVertices(paraboloidVertices);
    mesh.upload(m_allocator, m_graphicsQueue, raw(m_commandPool));
    const auto handle = m_meshes.insert(std::move(mesh));
    try
    {
        m_meshHandles.emplace(meshName, handle);
    }
    catch (...)
    {
        m_meshes.remove(handle);
        throw;
    }

    return meshName;
}

RenderObject *VulkanRenderer::getRenderObject(SparseSet<RenderObject>::Handle handle)
{
    return m_renderObjects.get(handle);
}
VulkanRenderer::~VulkanRenderer()
{
    try
    {
        cleanup();
    }
    catch (...)
    { /* Destructors must not throw on device loss. */
    }
}
