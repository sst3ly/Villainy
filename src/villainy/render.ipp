#include "render.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace vlny{

template <typename Vertex>
Renderer<Vertex>::Renderer(Context& context, Window& window, GraphicsPipeline& pipeline, Swapchain& swapchain) : context(context), window(window), pipeline(pipeline), swapchain(swapchain), commandPool(context){
    cmdBufs = commandPool.createCommandBuffers(context, window.getConfig().maxFramesInFlight);
}

template <typename Vertex>
void Renderer<Vertex>::drawFrame(){
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
    recordCommandBuffer(cmdBufs[currentFrame], imageIndex);

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

template<typename Vertex>
int Renderer<Vertex>::addRenderObject(RenderObject<Vertex> ro){
    renderObjects.push_back(ro);
    return renderObjects.size() - 1;
}

template<typename Vertex>
int Renderer<Vertex>::addRenderObject(VertexBuffer<Vertex>& vb, IndexBuffer& ib, UniformBuffer& ub){
    renderObjects.push_back({vb, ib, ub});
    return renderObjects.size() - 1;
}

template<typename Vertex>
int Renderer<Vertex>:: addRenderObjects(std::vector<RenderObject<Vertex>> ros){
    for(auto& ro : ros){
        renderObjects.push_back(ro);
    }
    return renderObjects.size() - ros.size();
}

template<typename Vertex>
void Renderer<Vertex>:: removeRenderObject(int index){
    renderObjects.erase(renderObjects.begin() + index);
}

template <typename Vertex>
void Renderer<Vertex>::recordCommandBuffer(CommandBuffer cmdBuf, uint32_t imageIndex){
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
        VkBuffer vertexBuffers[] = {ro.vb.vertexBuffer};
        VkBuffer indexBuffer = ro.ib.indexBuffer;
        auto& indices = ro.ib.indices;
        auto& descriptorSets = ro.ub.descriptorSets;
        VkDeviceSize offsets[] = {0};

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipelineLayout, 0, 
            1, &descriptorSets[currentFrame], 0, nullptr);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    }
    /*
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 
        1, &descriptorSets[currentFrame], 0, nullptr);
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);*/

    //vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    /*vertexCount: Even though we don't have a vertex buffer, we technically still have 3 vertices to draw.
    instanceCount: Used for instanced rendering, use 1 if you're not doing that.
    firstVertex: Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
    firstInstance: Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.*/

    vkCmdEndRenderPass(commandBuffer);
    
    if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS){
        throw std::runtime_error("failed to create command buffer!");
    }
}

}
