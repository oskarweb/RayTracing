#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <filesystem>
#include <string>
#include <vector>

// Value configuration: no pointers into its own storage, so presets can be
// copied and adjusted safely. Vertex input and shader interfaces belong to callers.
struct GraphicsPipelineConfig
{
    std::filesystem::path vertexShader;
    std::filesystem::path fragmentShader;
    std::string vertexEntryPoint = "main";
    std::string fragmentEntryPoint = "main";
    std::vector<vk::VertexInputBindingDescription> vertexBindings;
    std::vector<vk::VertexInputAttributeDescription> vertexAttributes;
    vk::RenderPass renderPass{};
    uint32_t subpass = 0;
    vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
    vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
    bool primitiveRestart = false;
    vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
    vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack;
    vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
    float lineWidth = 1.0f;
    bool depthTest = true;
    bool depthWrite = true;
    vk::CompareOp depthCompare = vk::CompareOp::eLess;
    std::vector<vk::PipelineColorBlendAttachmentState> colorAttachments;
};

// Creates a vertex/fragment graphics pipeline with dynamic viewport and scissor.
// The caller owns the layout and supplies settings supported by the enabled device features.
vk::raii::Pipeline createGraphicsPipeline(const vk::raii::Device &device, vk::PipelineLayout layout,
                                          const GraphicsPipelineConfig &config);
