#ifndef VILLAINY_BUFFER_HPP
#define VILLAINY_BUFFER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <stdint.h>
#include <cstring>

#include "logger.hpp"
#include "context.hpp"
#include "window.hpp"

namespace vlny{

class Texture;
class Sampler;
class CommandBuffer;
class Swapchain;
template<typename Vertex> class Renderer;

template<typename Vertex>
struct VertexBuffer{
    VertexBuffer(Context& context, std::vector<Vertex> vertices);
    
    void updateBuffer(const void* data, size_t size);

    ~VertexBuffer();
private:
    Context& context;

    std::vector<Vertex> vertices;

    VkDeviceSize bufferSize;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;

    template<typename V> friend class Renderer;
};

struct IndexBuffer{
    IndexBuffer(Context& context, std::vector<uint16_t> indices);
    ~IndexBuffer();
private:
    Context& context;

    std::vector<uint16_t> indices;

    VkDeviceSize bufferSize;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

    template<typename V> friend class Renderer;
};

struct UniformBuffer {
    UniformBuffer(Context& context, WindowConfig windowconfig, size_t bufferSize, std::vector<VkDescriptorPoolSize> poolSizes);
    ~UniformBuffer();

    // Delete copy constructor and assignment operator
    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;

    // Public interface
    void updateBuffer(uint32_t currentFrame, const void* data);
    void addBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags);
    void createDescriptorSetLayout();
    void allocateDescriptorSets();
    void updateDescriptorSets();
    
    VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }
    VkDescriptorSet getDescriptorSet(uint32_t frame) const { return descriptorSets[frame]; }
    void* getMappedMemory(uint32_t frame) const { return uniformBuffersMapped[frame]; }

private:
    Context& context;
    uint32_t maxFramesInFlight;

    VkDeviceSize bufferSize;
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    std::vector<VkDescriptorSet> descriptorSets;

    std::vector<VkDescriptorPoolSize> poolSizes;
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    
    void createUniformBuffers();
    void createDescriptorPool();
    void cleanup();

    template<typename V> friend class Renderer;
};


void copyBuffer(Context& context, VkBuffer srcBuf, VkBuffer dstBuf, VkDeviceSize size);
void copyBufferToImage(Context& context, CommandPool commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
void createBuffer(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

}

#include "buffer.ipp"

#endif
