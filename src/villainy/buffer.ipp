#include "buffer.hpp"
//#include "context.hpp"

namespace vlny{

template <typename Vertex>
VertexBuffer<Vertex>::VertexBuffer(Context& context, std::vector<Vertex> vertices) : context(context), vertices(vertices) {
    bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(context.getLogicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(context.getLogicalDevice(), stagingBufferMemory);
    
    createBuffer(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexBuffer, vertexBufferMemory);
    
    copyBuffer(context, stagingBuffer, vertexBuffer, bufferSize);
    
    vkDestroyBuffer(context.getLogicalDevice(), stagingBuffer, nullptr);
    vkFreeMemory(context.getLogicalDevice(), stagingBufferMemory, nullptr);
}

template <typename Vertex>
void VertexBuffer<Vertex>::updateBuffer(const void* dataPtr, size_t size){
    void* data;
    vkMapMemory(context.getLogicalDevice(),
                vertexBufferMemory,
                0,
                size,
                0,
                &data);

    memcpy(data, dataPtr, size);

    vkUnmapMemory(context.getLogicalDevice(),
                  vertexBufferMemory);
}

/*
template <typename Vertex>
void VertexBuffer<Vertex>::updateBuffer(const void* dataPtr, size_t size){
    if(size == 0) return; 

    // update CPU-side vertices
    size_t numVertices = size / sizeof(Vertex);
    vertices.resize(numVertices);
    memcpy(vertices.data(), dataPtr, size);

    // recreate buffer if needed
    if(size > bufferSize){
        if(vertexBuffer != VK_NULL_HANDLE){
            vkDestroyBuffer(context.getLogicalDevice(), vertexBuffer, nullptr);
            vertexBuffer = VK_NULL_HANDLE;
        }
        if(vertexBufferMemory != VK_NULL_HANDLE){
            vkFreeMemory(context.getLogicalDevice(), vertexBufferMemory, nullptr);
            vertexBufferMemory = VK_NULL_HANDLE;
        }

        bufferSize = size;
        createBuffer(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexBuffer, vertexBufferMemory);
    }
    else{
        bufferSize = size;
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    void* data;
    vkMapMemory(context.getLogicalDevice(), stagingBufferMemory, 0, size, 0, &data);
    memcpy(data, dataPtr, size);
    vkUnmapMemory(context.getLogicalDevice(), stagingBufferMemory);

    copyBuffer(context, stagingBuffer, vertexBuffer, size);


    vkDestroyBuffer(context.getLogicalDevice(), stagingBuffer, nullptr);
    vkFreeMemory(context.getLogicalDevice(), stagingBufferMemory, nullptr);
}*/

template <typename Vertex>
VertexBuffer<Vertex>::~VertexBuffer(){
    if(vertexBuffer != VK_NULL_HANDLE){
        vkDestroyBuffer(context.getLogicalDevice(), vertexBuffer, nullptr);
        vertexBuffer = VK_NULL_HANDLE;
    }
    if(vertexBufferMemory != VK_NULL_HANDLE){
        vkFreeMemory(context.getLogicalDevice(), vertexBufferMemory, nullptr);
        vertexBufferMemory = VK_NULL_HANDLE;
    }
}

}
