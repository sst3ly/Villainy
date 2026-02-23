#include "descriptorManager.hpp"

#include "context.hpp"
#include "texture.hpp"

namespace vlny{

DescriptorManager::DescriptorManager(Context& context) : context(context), maxFramesInFlight(context.config.maxFramesInFlight) {}

DescriptorManager::~DescriptorManager() {
    cleanup();
}

DescriptorManager& DescriptorManager::addUniformBuffer(uint32_t binding, VkShaderStageFlags stageFlags, 
                                                        VkDeviceSize bufferSize) {
    if (isBuilt) {
        throw std::runtime_error("Cannot add bindings after build() has been called!");
    }
    
    DescriptorBinding desc;
    desc.binding = binding;
    desc.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    desc.stageFlags = stageFlags;
    desc.bufferSize = bufferSize;
    
    bindings.push_back(desc);
    return *this;
}

DescriptorManager& DescriptorManager::addCombinedImageSampler(uint32_t binding, VkShaderStageFlags stageFlags,
                                                               std::shared_ptr<Texture> texture, 
                                                               std::shared_ptr<Sampler> sampler) {
    if (isBuilt) {
        throw std::runtime_error("Cannot add bindings after build() has been called!");
    }
    
    DescriptorBinding desc;
    desc.binding = binding;
    desc.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    desc.stageFlags = stageFlags;
    desc.texture = texture;
    desc.sampler = sampler;
    
    bindings.push_back(desc);
    return *this;
}

DescriptorManager& DescriptorManager::addStorageBuffer(uint32_t binding, VkShaderStageFlags stageFlags, 
                                                        VkDeviceSize bufferSize) {
    if (isBuilt) {
        throw std::runtime_error("Cannot add bindings after build() has been called!");
    }
    
    DescriptorBinding desc;
    desc.binding = binding;
    desc.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    desc.stageFlags = stageFlags;
    desc.bufferSize = bufferSize;
    
    bindings.push_back(desc);
    return *this;
}

DescriptorManager& DescriptorManager::addStorageImage(uint32_t binding, VkShaderStageFlags stageFlags,
                                                       std::shared_ptr<Texture> texture) {
    if (isBuilt) {
        throw std::runtime_error("Cannot add bindings after build() has been called!");
    }
    
    DescriptorBinding desc;
    desc.binding = binding;
    desc.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    desc.stageFlags = stageFlags;
    desc.texture = texture;
    
    bindings.push_back(desc);
    return *this;
}

DescriptorManager& DescriptorManager::addExternalBuffer(uint32_t binding, VkShaderStageFlags stageFlags,
                                                         VkDescriptorType type, VkBuffer buffer, 
                                                         VkDeviceSize size) {
    if (isBuilt) {
        throw std::runtime_error("Cannot add bindings after build() has been called!");
    }
    
    DescriptorBinding desc;
    desc.binding = binding;
    desc.type = type;
    desc.stageFlags = stageFlags;
    desc.externalBuffer = buffer;
    desc.externalBufferSize = size;
    
    bindings.push_back(desc);
    return *this;
}

void DescriptorManager::build() {
    if (isBuilt) {
        throw std::runtime_error("build() has already been called!");
    }
    
    if (bindings.empty()) {
        throw std::runtime_error("No bindings added before build()!");
    }
    
    // Create buffers for uniform and storage buffers
    for (const auto& binding : bindings) {
        if (binding.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
            createUniformBuffer(binding.binding, binding.bufferSize);
        } else if (binding.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
            createStorageBuffer(binding.binding, binding.bufferSize);
        }
    }
    
    createDescriptorSetLayout();
    createDescriptorPool();
    allocateDescriptorSets();
    updateDescriptorSets();
    
    isBuilt = true;
}

void DescriptorManager::createUniformBuffer(uint32_t binding, VkDeviceSize bufferSize) {
    UniformBufferData ubo;
    ubo.buffers.resize(maxFramesInFlight);
    ubo.memory.resize(maxFramesInFlight);
    ubo.mapped.resize(maxFramesInFlight);
    
    for (uint32_t i = 0; i < maxFramesInFlight; i++) {
        createBuffer(context, bufferSize,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    ubo.buffers[i], ubo.memory[i]);
        
        vkMapMemory(context.logicalDevice, ubo.memory[i], 0, bufferSize, 0, &ubo.mapped[i]);
    }
    
    uniformBuffers[binding] = std::move(ubo);
}

void DescriptorManager::createStorageBuffer(uint32_t binding, VkDeviceSize bufferSize) {
    StorageBufferData sbo;
    sbo.buffers.resize(maxFramesInFlight);
    sbo.memory.resize(maxFramesInFlight);
    sbo.mapped.resize(maxFramesInFlight);
    
    for (uint32_t i = 0; i < maxFramesInFlight; i++) {
        createBuffer(context, bufferSize,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    sbo.buffers[i], sbo.memory[i]);
        
        vkMapMemory(context.logicalDevice, sbo.memory[i], 0, bufferSize, 0, &sbo.mapped[i]);
    }
    
    storageBuffers[binding] = std::move(sbo);
}

void DescriptorManager::createDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
    
    for (const auto& binding : bindings) {
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding.binding;
        layoutBinding.descriptorType = binding.type;
        layoutBinding.descriptorCount = binding.descriptorCount;
        layoutBinding.stageFlags = binding.stageFlags;
        layoutBinding.pImmutableSamplers = nullptr;
        
        layoutBindings.push_back(layoutBinding);
    }
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    layoutInfo.pBindings = layoutBindings.data();
    
    if (vkCreateDescriptorSetLayout(context.logicalDevice, &layoutInfo, nullptr, 
                                   &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
}

void DescriptorManager::createDescriptorPool() {
    // Count descriptor types
    std::map<VkDescriptorType, uint32_t> descriptorCounts;
    for (const auto& binding : bindings) {
        descriptorCounts[binding.type] += binding.descriptorCount;
    }
    
    // Create pool sizes
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (const auto& [type, count] : descriptorCounts) {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = type;
        poolSize.descriptorCount = count * maxFramesInFlight;
        poolSizes.push_back(poolSize);
    }
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxFramesInFlight;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // Allow updating
    
    if (vkCreateDescriptorPool(context.logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}

void DescriptorManager::allocateDescriptorSets() {
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

void DescriptorManager::updateDescriptorSets() {
    for (uint32_t frame = 0; frame < maxFramesInFlight; frame++) {
        std::vector<VkWriteDescriptorSet> descriptorWrites;
        std::vector<VkDescriptorBufferInfo> bufferInfos;
        std::vector<VkDescriptorImageInfo> imageInfos;
        
        // Reserve space to prevent reallocation
        bufferInfos.reserve(bindings.size());
        imageInfos.reserve(bindings.size());
        
        for (const auto& binding : bindings) {
            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSets[frame];
            descriptorWrite.dstBinding = binding.binding;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = binding.type;
            descriptorWrite.descriptorCount = binding.descriptorCount;
            
            if (binding.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                VkDescriptorBufferInfo bufferInfo{};
                bufferInfo.buffer = uniformBuffers[binding.binding].buffers[frame];
                bufferInfo.offset = 0;
                bufferInfo.range = binding.bufferSize;
                bufferInfos.push_back(bufferInfo);
                descriptorWrite.pBufferInfo = &bufferInfos.back();
            }
            else if (binding.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
                VkDescriptorBufferInfo bufferInfo{};
                bufferInfo.buffer = storageBuffers[binding.binding].buffers[frame];
                bufferInfo.offset = 0;
                bufferInfo.range = binding.bufferSize;
                bufferInfos.push_back(bufferInfo);
                descriptorWrite.pBufferInfo = &bufferInfos.back();
            }
            else if (binding.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                VkDescriptorImageInfo imageInfo{};
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo.imageView = binding.texture->imageView;
                imageInfo.sampler = binding.sampler->vkSampler;
                imageInfos.push_back(imageInfo);
                descriptorWrite.pImageInfo = &imageInfos.back();
            }
            else if (binding.type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
                VkDescriptorImageInfo imageInfo{};
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                imageInfo.imageView = binding.texture->imageView;
                imageInfo.sampler = VK_NULL_HANDLE;
                imageInfos.push_back(imageInfo);
                descriptorWrite.pImageInfo = &imageInfos.back();
            }
            else if (binding.externalBuffer != VK_NULL_HANDLE) {
                VkDescriptorBufferInfo bufferInfo{};
                bufferInfo.buffer = binding.externalBuffer;
                bufferInfo.offset = 0;
                bufferInfo.range = binding.externalBufferSize;
                bufferInfos.push_back(bufferInfo);
                descriptorWrite.pBufferInfo = &bufferInfos.back();
            }
            
            descriptorWrites.push_back(descriptorWrite);
        }
        
        vkUpdateDescriptorSets(context.logicalDevice, 
                              static_cast<uint32_t>(descriptorWrites.size()),
                              descriptorWrites.data(), 0, nullptr);
    }
}

void DescriptorManager::updateUniformBuffer(uint32_t binding, uint32_t frame, const void* data, 
                                           VkDeviceSize size) {
    if (!isBuilt) {
        throw std::runtime_error("Must call build() before updating buffers!");
    }
    
    auto it = uniformBuffers.find(binding);
    if (it == uniformBuffers.end()) {
        throw std::runtime_error("Uniform buffer binding not found!");
    }
    
    // Find the actual buffer size
    VkDeviceSize actualSize = size;
    if (size == 0) {
        for (const auto& b : bindings) {
            if (b.binding == binding && b.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                actualSize = b.bufferSize;
                break;
            }
        }
    }
    
    memcpy(it->second.mapped[frame], data, static_cast<size_t>(actualSize));
}

void DescriptorManager::updateImageSampler(uint32_t binding, uint32_t frame, 
                                          std::shared_ptr<Texture> texture,
                                          std::shared_ptr<Sampler> sampler) {
    if (!isBuilt) {
        throw std::runtime_error("Must call build() before updating image samplers!");
    }
    
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = texture->imageView;
    imageInfo.sampler = sampler->vkSampler;
    
    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSets[frame];
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;
    
    vkUpdateDescriptorSets(context.logicalDevice, 1, &descriptorWrite, 0, nullptr);
}

void* DescriptorManager::getMappedUniformBuffer(uint32_t binding, uint32_t frame) const {
    auto it = uniformBuffers.find(binding);
    if (it == uniformBuffers.end()) {
        throw std::runtime_error("Uniform buffer binding not found!");
    }
    return it->second.mapped[frame];
}

VkBuffer DescriptorManager::getUniformBuffer(uint32_t binding, uint32_t frame) const {
    auto it = uniformBuffers.find(binding);
    if (it == uniformBuffers.end()) {
        throw std::runtime_error("Uniform buffer binding not found!");
    }
    return it->second.buffers[frame];
}

void DescriptorManager::cleanup() {
    // Clean up uniform buffers
    for (auto& [binding, ubo] : uniformBuffers) {
        for (uint32_t i = 0; i < maxFramesInFlight; i++) {
            if (ubo.mapped[i] != nullptr) {
                vkUnmapMemory(context.logicalDevice, ubo.memory[i]);
            }
            if (ubo.buffers[i] != VK_NULL_HANDLE) {
                vkDestroyBuffer(context.logicalDevice, ubo.buffers[i], nullptr);
            }
            if (ubo.memory[i] != VK_NULL_HANDLE) {
                vkFreeMemory(context.logicalDevice, ubo.memory[i], nullptr);
            }
        }
    }
    
    // Clean up storage buffers
    for (auto& [binding, sbo] : storageBuffers) {
        for (uint32_t i = 0; i < maxFramesInFlight; i++) {
            if (sbo.mapped[i] != nullptr) {
                vkUnmapMemory(context.logicalDevice, sbo.memory[i]);
            }
            if (sbo.buffers[i] != VK_NULL_HANDLE) {
                vkDestroyBuffer(context.logicalDevice, sbo.buffers[i], nullptr);
            }
            if (sbo.memory[i] != VK_NULL_HANDLE) {
                vkFreeMemory(context.logicalDevice, sbo.memory[i], nullptr);
            }
        }
    }
    
    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(context.logicalDevice, descriptorPool, nullptr);
    }
    
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(context.logicalDevice, descriptorSetLayout, nullptr);
    }
}

}