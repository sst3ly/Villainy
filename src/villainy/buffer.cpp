#include "buffer.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "context.hpp"
#include "logger.hpp"
#include "command.hpp"

namespace vlny{

IndexBuffer::IndexBuffer(Context& context, std::vector<uint16_t> indices) : context(context), indices(indices){
    bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(context.logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t) bufferSize);
    vkUnmapMemory(context.logicalDevice, stagingBufferMemory);
    
    createBuffer(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
    
    copyBuffer(context, stagingBuffer, indexBuffer, bufferSize);
    
    vkDestroyBuffer(context.logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(context.logicalDevice, stagingBufferMemory, nullptr);
}

IndexBuffer::~IndexBuffer(){
    if(indexBuffer != VK_NULL_HANDLE){
        vkDestroyBuffer(context.logicalDevice, indexBuffer, nullptr);
        indexBuffer = VK_NULL_HANDLE;
    }
    if(indexBufferMemory != VK_NULL_HANDLE){
        vkFreeMemory(context.logicalDevice, indexBufferMemory, nullptr);
        indexBufferMemory = VK_NULL_HANDLE;
    }
}
/*
UniformBufferDescriptorSetBuilder::UniformBufferDescriptorSetBuilder(Context& context) : context(context) {
    descriptorCount = context.config.maxFramesInFlight;
}

void UniformBufferDescriptorSetBuilder::addTextureSampler(Sampler& sampler, Texture& texture){
    VkDescriptorPoolSize dps{};
    dps.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    dps.descriptorCount = descriptorCount;
    poolSizes.push_back(dps);
}

void UniformBufferDescriptorSetBuilder::build(){
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = scast_ui32(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = scast_ui32(descriptorCount);

    if(vkCreateDescriptorPool(context.logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS){
        throw std::runtime_error("Failed to make descriptor pool!");
    }
}*/

UniformBuffer::UniformBuffer(Context& context, size_t bufferSize, std::vector<VkDescriptorPoolSize> poolSizes)
    : context(context)
    , maxFramesInFlight(context.config.maxFramesInFlight)
    , bufferSize(bufferSize)
    , poolSizes(std::move(poolSizes))
    , descriptorPool(VK_NULL_HANDLE)
    , descriptorSetLayout(VK_NULL_HANDLE)
{
    createUniformBuffers();
}

UniformBuffer::~UniformBuffer() {
    cleanup();
}

void UniformBuffer::createUniformBuffers() {
    uniformBuffers.resize(maxFramesInFlight);
    uniformBuffersMemory.resize(maxFramesInFlight);
    uniformBuffersMapped.resize(maxFramesInFlight);

    for (size_t i = 0; i < maxFramesInFlight; i++) {
        createBuffer(context, bufferSize, 
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    uniformBuffers[i], uniformBuffersMemory[i]);

        // Map the buffer memory so we can write to it
        vkMapMemory(context.logicalDevice, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}

void UniformBuffer::updateBuffer(uint32_t currentFrame, const void* data) {
    memcpy(uniformBuffersMapped[currentFrame], data, static_cast<size_t>(bufferSize));
}

void UniformBuffer::addBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags) {
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = descriptorType;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = stageFlags;
    layoutBinding.pImmutableSamplers = nullptr;

    bindings.push_back(layoutBinding);
}

void UniformBuffer::createDescriptorSetLayout() {
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(context.logicalDevice, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
}

void UniformBuffer::createDescriptorPool() {
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxFramesInFlight;

    if (vkCreateDescriptorPool(context.logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}

void UniformBuffer::allocateDescriptorSets() {
    if (descriptorPool == VK_NULL_HANDLE) {
        createDescriptorPool();
    }

    std::vector<VkDescriptorSetLayout> layouts(maxFramesInFlight, descriptorSetLayout);
    
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = maxFramesInFlight;
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(maxFramesInFlight);
    if (vkAllocateDescriptorSets(context.logicalDevice, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }
}

void UniformBuffer::updateDescriptorSets() {
    for (size_t i = 0; i < maxFramesInFlight; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = bufferSize;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = 0;  // Assuming binding 0, adjust as needed
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(context.logicalDevice, 1, &descriptorWrite, 0, nullptr);
    }
}

void UniformBuffer::cleanup() {
    // Unmap memory
    for (size_t i = 0; i < maxFramesInFlight; i++) {
        if (uniformBuffersMapped[i] != nullptr) {
            vkUnmapMemory(context.logicalDevice, uniformBuffersMemory[i]);
        }
    }

    // Destroy buffers
    for (size_t i = 0; i < maxFramesInFlight; i++) {
        if (uniformBuffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer(context.logicalDevice, uniformBuffers[i], nullptr);
        }
        if (uniformBuffersMemory[i] != VK_NULL_HANDLE) {
            vkFreeMemory(context.logicalDevice, uniformBuffersMemory[i], nullptr);
        }
    }

    // Destroy descriptor pool (this also frees descriptor sets)
    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(context.logicalDevice, descriptorPool, nullptr);
    }

    // Destroy descriptor set layout
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(context.logicalDevice, descriptorSetLayout, nullptr);
    }
}


// ------------------------------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------------------------------

void copyBuffer(Context& context, VkBuffer srcBuf, VkBuffer dstBuf, VkDeviceSize size){
    CommandBuffer cmdBuf(context, context.getTransientCommandPool());
    cmdBuf.beginSingletimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(cmdBuf.vkCommandBuffer, srcBuf, dstBuf, 1, &copyRegion);

    cmdBuf.endSingletimeCommands();
}
void copyBufferToImage(Context& context, CommandPool commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height){
    CommandBuffer cmdBuf(context, commandPool);
    cmdBuf.beginSingletimeCommands();

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

    vkCmdCopyBufferToImage(cmdBuf.vkCommandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    
    cmdBuf.endSingletimeCommands();
}
void createBuffer(Context& context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory){
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if(vkCreateBuffer(context.logicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS){
        throw std::runtime_error("Failed to create buffer!");
    }
    VILLAINY_VERBOSE_LOG(context.logger, "Made buffer");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(context.logicalDevice, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(context.physicalDevice, memRequirements.memoryTypeBits, properties);

    if(vkAllocateMemory(context.logicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS){
        throw std::runtime_error("Failed to allocate buffer memory!");
    }

    VILLAINY_VERBOSE_LOG(context.logger, "Allocated buffer memory.");

    vkBindBufferMemory(context.logicalDevice, buffer, bufferMemory, 0);
}

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties){
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++){
        if(typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties){
            return i;
        }
    }
    
    throw std::runtime_error("Failed to find a suitable memory type!");
}

}
