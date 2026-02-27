#include "render.hpp"

#include "command.hpp"
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

Renderer::Renderer(Context& context, Window& window, Swapchain& swapchain) : context(context), window(window),   swapchain(swapchain), commandPool(context) {
    cmdBufs = commandPool.createCommandBuffers(context, window.getConfig().maxFramesInFlight);
}

void Renderer::drawFrame(GraphicsPipeline& pipeline){
    vkWaitForFences(context.logicalDevice, 1, &swapchain.inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(context.logicalDevice, swapchain.vkSwapchain, UINT64_MAX, swapchain.imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    if(result == VK_ERROR_OUT_OF_DATE_KHR){
        // TODO: implement recreate swapchain + call it here
        // TODO: tell window that framebuffer has been resized to manually reset it for the new swapchain
        return;
    }
    else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    vkResetFences(context.logicalDevice, 1, &swapchain.inFlightFences[currentFrame]);
    vkResetCommandBuffer(cmdBufs[currentFrame].vkCommandBuffer, 0);
    recordCommandBuffer(cmdBufs[currentFrame], imageIndex, pipeline);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {swapchain.imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBufs[currentFrame].vkCommandBuffer;

    VkSemaphore signalSemaphores[] = {swapchain.renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if(vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, swapchain.inFlightFences[currentFrame]) != VK_SUCCESS){
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = {swapchain.vkSwapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr; // optional

    result = vkQueuePresentKHR(context.presentQueue, &presentInfo);
    // Demo-safe synchronization: serialize presents so per-frame semaphores are not reused
    // while the swapchain may still be consuming them.
    vkQueueWaitIdle(context.presentQueue);
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || swapchain.framebufferResized){
        swapchain.framebufferResized = false;
        swapchain.recreateSwapchain();
    } else if(result != VK_SUCCESS){
        throw std::runtime_error("Failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % window.getConfig().maxFramesInFlight;
}

int Renderer::addRenderObject(std::unique_ptr<RenderObjectBase> ro){
    renderObjects.push_back(std::move(ro));
    return static_cast<int>(renderObjects.size() - 1);
}

int Renderer::addRenderObjects(std::vector<std::unique_ptr<RenderObjectBase>> ros){
    for(auto& ro : ros){
        renderObjects.push_back(std::move(ro));
    }
    return renderObjects.size() - static_cast<int>(ros.size());
}

void Renderer::removeRenderObject(int index){
    renderObjects.erase(renderObjects.begin() + index);
}

void Renderer::recordCommandBuffer(CommandBuffer cmdBuf, uint32_t imageIndex, GraphicsPipeline& pipeline){
    VkCommandBuffer commandBuffer = cmdBuf.vkCommandBuffer;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    /*VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: The command buffer will be rerecorded right after executing it once.
    VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: This is a secondary command buffer that will be entirely within 
        a single render pass.
    VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: The command buffer can be resubmitted while it is also already 
        pending execution.*/
    beginInfo.pInheritanceInfo = nullptr; // only for secondary command buffers

    if(vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS){
        throw std::runtime_error("Failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = swapchain.renderPass->vkRenderPass;
    renderPassInfo.framebuffer = swapchain.swapchainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchain.swapchainExtent;

    
    WindowConfig windowConfig = window.getConfig();
    VkClearValue clearColor = {{{static_cast<float>(windowConfig.clearColor[0]),
        static_cast<float>(windowConfig.clearColor[1]), static_cast<float>(windowConfig.clearColor[2]), 1.0f}}};
    // ENDED HERE <----------------------------------------------------
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    /*VK_SUBPASS_CONTENTS_INLINE: The render pass commands will be embedded in the primary command buffer itself and no secondary 
        command buffers will be executed.
    VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: The render pass commands will be executed from secondary command buffers.*/

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.graphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchain.swapchainExtent.width);
    viewport.height = static_cast<float>(swapchain.swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchain.swapchainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    for(const auto& ro : renderObjects){
        const auto& renderObject = ro.get();
        renderObject->draw(commandBuffer, pipeline, currentFrame);
    }

    vkCmdEndRenderPass(commandBuffer);
    
    if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS){
        throw std::runtime_error("failed to create command buffer!");
    }
}

}
