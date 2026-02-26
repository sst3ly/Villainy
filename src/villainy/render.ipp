#pragma once

#include "render.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace vlny{

template <typename Vertex>
RenderObject<Vertex>::RenderObject(VertexBuffer<Vertex>& vb, IndexBuffer& ib, UniformBuffer& ub) : vb(vb), ib(ib), ub(ub) {}

template <typename Vertex>
void RenderObject<Vertex>::draw(VkCommandBuffer commandBuffer){
    VkBuffer vertexBuffers[] = {vb.vertexBuffer};
    VkBuffer indexBuffer = ib.indexBuffer;
    auto& indices = ib.indices;

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

template <typename Vertex>
int Renderer::addRenderObject(RenderObject<Vertex>& ro){
    renderObjects.push_back(std::make_unique<RenderObject<Vertex>>(ro));
    return static_cast<int>(renderObjects.size() - 1);
}

}