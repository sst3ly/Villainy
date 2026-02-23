#include "render.hpp"

#include "window.hpp"
#include "swapchain.hpp"
#include "context.hpp"
#include "buffer.hpp"

namespace vlny{

GraphicsPipeline::GraphicsPipeline(GraphicsPipelineConfig config, Context& context, Swapchain& swapchain, ShaderProgram& shaderProgram) : config(config), context(context), swapchain(swapchain), shaderProgram(shaderProgram) {
    init();
}

GraphicsPipeline::GraphicsPipeline(Swapchain& swapchain, Context& context, ShaderProgram& shaderProgram) : context(context), swapchain(swapchain), shaderProgram(shaderProgram) {
    init();
}

GraphicsPipeline::~GraphicsPipeline(){
    if(graphicsPipeline != VK_NULL_HANDLE){
        vkDestroyPipeline(context.logicalDevice, graphicsPipeline, nullptr);
        graphicsPipeline = VK_NULL_HANDLE;
    }
    if(pipelineLayout != VK_NULL_HANDLE){
        vkDestroyPipelineLayout(context.logicalDevice, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }
}

void GraphicsPipeline::init(){
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = scast_ui32(config.dynamicStates.size());
    dynamicState.pDynamicStates = config.dynamicStates.data();

    VkVertexInputBindingDescription vertexBindingDesc{};
    vertexBindingDesc.binding = config.vertexData.binding;
    vertexBindingDesc.inputRate = config.vertexData.inputRate;
    vertexBindingDesc.stride = config.vertexData.stride;

    uint32_t numVertexAttribs = scast_ui32(config.vertexData.vertexAttributes.size());
    std::vector<VkVertexInputAttributeDescription> vertexAttribs(numVertexAttribs);
    for(int i = 0; i < numVertexAttribs; i++){
        vertexAttribs[i].binding = config.vertexData.binding;
        vertexAttribs[i].location = i;
        vertexAttribs[i].format = config.vertexData.vertexAttributes[i].first;
        vertexAttribs[i].offset = config.vertexData.vertexAttributes[i].second;
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = numVertexAttribs;
    vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDesc;
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttribs.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = config.vertexData.topology;
    inputAssemblyInfo.primitiveRestartEnable = config.vertexData.primitiveRestart;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.height = swapchain.window.config.height;
    viewport.width = swapchain.window.config.width;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchain.swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = config.depthClamp;
    rasterizer.polygonMode = config.polygonMode;
    rasterizer.lineWidth = config.lineWidth;
    rasterizer.cullMode = config.cullMode;
    rasterizer.frontFace = config.frontFace;
    // TODO: decice if the following 4 should be in config
    rasterizer.depthBiasClamp = false;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    // TODO: implement multisampling + add appropriate configs
    VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
    multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingInfo.sampleShadingEnable = VK_FALSE;
    multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisamplingInfo.minSampleShading = 1.0f;
    multisamplingInfo.pSampleMask = nullptr;
    multisamplingInfo.alphaToCoverageEnable = VK_FALSE;
    multisamplingInfo.alphaToOneEnable = VK_FALSE;

    // TODO: add to configs
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; 
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; 
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; 
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f; 

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &shaderProgram.descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if(vkCreatePipelineLayout(context.logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS){
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    VILLAINY_VERBOSE_LOG(context.logger, "Made graphics pipeline layout.");

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = shaderProgram.numShaders;
    pipelineInfo.pStages = shaderProgram.vkShaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisamplingInfo;
    pipelineInfo.pDepthStencilState = nullptr; // TODO: look into
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;

    pipelineInfo.renderPass = swapchain.renderPass.value().vkRenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if(vkCreateGraphicsPipelines(context.logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS){
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    VILLAINY_VERBOSE_LOG(context.logger, "Made graphics pipeline.");
}

}
