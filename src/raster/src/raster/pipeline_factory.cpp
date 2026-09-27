#include "raster/pipeline_factory.hpp"

#include "core/extras.hpp"
#include "core/vulkan/commands.hpp"

#include <array>

vk::raii::Pipeline createGraphicsPipeline(const vk::raii::Device &device, vk::PipelineLayout layout,
                                          const GraphicsPipelineConfig &config)
{
    auto vertexShader = Helpers::createShaderModule(device, Helpers::readFile(config.vertexShader));
    auto fragmentShader = Helpers::createShaderModule(device, Helpers::readFile(config.fragmentShader));
    const std::array stages = {vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, *vertexShader,
                                                                 config.vertexEntryPoint.c_str()),
                               vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment,
                                                                 *fragmentShader, config.fragmentEntryPoint.c_str())};

    vk::PipelineVertexInputStateCreateInfo vertexInput;
    vertexInput.setVertexBindingDescriptions(config.vertexBindings)
        .setVertexAttributeDescriptions(config.vertexAttributes);
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly({}, config.topology, config.primitiveRestart);
    vk::PipelineViewportStateCreateInfo viewport;
    viewport.setViewportCount(1).setScissorCount(1);
    vk::PipelineRasterizationStateCreateInfo rasterizer;
    rasterizer.setPolygonMode(config.polygonMode)
        .setCullMode(config.cullMode)
        .setFrontFace(config.frontFace)
        .setLineWidth(config.lineWidth);
    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.setRasterizationSamples(config.samples).setMinSampleShading(1.0f);
    vk::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.setDepthTestEnable(config.depthTest)
        .setDepthWriteEnable(config.depthWrite)
        .setDepthCompareOp(config.depthCompare);
    vk::PipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.setAttachments(config.colorAttachments);
    const std::array dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicState;
    dynamicState.setDynamicStates(dynamicStates);

    // These pointers only borrow local/configuration storage for the synchronous
    // Vulkan creation call. The returned pipeline owns no references to config.
    vk::GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.setStages(stages)
        .setPVertexInputState(&vertexInput)
        .setPInputAssemblyState(&inputAssembly)
        .setPViewportState(&viewport)
        .setPRasterizationState(&rasterizer)
        .setPMultisampleState(&multisampling)
        .setPDepthStencilState(&depthStencil)
        .setPColorBlendState(&colorBlending)
        .setPDynamicState(&dynamicState)
        .setLayout(layout)
        .setRenderPass(config.renderPass)
        .setSubpass(config.subpass)
        .setBasePipelineIndex(-1);
    return vk::raii::Pipeline(device, nullptr, pipelineInfo);
}
