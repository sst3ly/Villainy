#ifndef VILLAINY_DESCRIPTOR_MANAGER
#define VILLAINY_DESCRIPTOR_MANAGER

#include <cstdint>
#include <stdexcept>
#include <map>
#include <vector>
#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "window.hpp"

namespace vlny{

class Context;
class Sampler;
class Texture;

struct DescriptorBinding {
    uint32_t binding;
    VkDescriptorType type;
    VkShaderStageFlags stageFlags;
    uint32_t descriptorCount = 1;
    
    // For uniform buffers
    VkDeviceSize bufferSize = 0;
    
    // For combined image samplers
    std::shared_ptr<Texture> texture = nullptr;
    std::shared_ptr<Sampler> sampler = nullptr;
    
    // For storage buffers or other buffer types
    VkBuffer externalBuffer = VK_NULL_HANDLE;
    VkDeviceSize externalBufferSize = 0;
};

class DescriptorManager {
public:
    DescriptorManager(Context& context, Window& window);
    ~DescriptorManager();

    // Delete copy constructor and assignment operator
    DescriptorManager(const DescriptorManager&) = delete;
    DescriptorManager& operator=(const DescriptorManager&) = delete;

    // Fluid API for adding bindings
    DescriptorManager& addUniformBuffer(uint32_t binding, VkShaderStageFlags stageFlags, VkDeviceSize bufferSize);
    DescriptorManager& addCombinedImageSampler(uint32_t binding, VkShaderStageFlags stageFlags, 
                                               std::shared_ptr<Texture> texture, std::shared_ptr<Sampler> sampler);
    DescriptorManager& addStorageBuffer(uint32_t binding, VkShaderStageFlags stageFlags, VkDeviceSize bufferSize);
    DescriptorManager& addStorageImage(uint32_t binding, VkShaderStageFlags stageFlags, 
                                       std::shared_ptr<Texture> texture);
    DescriptorManager& addExternalBuffer(uint32_t binding, VkShaderStageFlags stageFlags, 
                                         VkDescriptorType type, VkBuffer buffer, VkDeviceSize size);
    
    // Build the descriptor sets (call after all bindings are added)
    void build();
    
    // Update uniform buffer data
    void updateUniformBuffer(uint32_t binding, uint32_t frame, const void* data, VkDeviceSize size = 0);
    
    // Update texture/sampler after build (if you want to swap textures)
    void updateImageSampler(uint32_t binding, uint32_t frame, std::shared_ptr<Texture> texture, 
                           std::shared_ptr<Sampler> sampler);
    
    // Getters
    VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }
    VkDescriptorSet getDescriptorSet(uint32_t frame) const { return descriptorSets[frame]; }
    void* getMappedUniformBuffer(uint32_t binding, uint32_t frame) const;
    
    // Get buffer for external use (e.g., if you want direct access)
    VkBuffer getUniformBuffer(uint32_t binding, uint32_t frame) const;

private:
    Context& context;
    uint32_t maxFramesInFlight;
    
    std::vector<DescriptorBinding> bindings;
    bool isBuilt = false;
    
    // Descriptor resources
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;
    
    // Uniform buffer storage (per binding, per frame)
    struct UniformBufferData {
        std::vector<VkBuffer> buffers;
        std::vector<VkDeviceMemory> memory;
        std::vector<void*> mapped;
    };
    std::map<uint32_t, UniformBufferData> uniformBuffers; // key = binding
    
    // Storage buffer storage (per binding, per frame)
    struct StorageBufferData {
        std::vector<VkBuffer> buffers;
        std::vector<VkDeviceMemory> memory;
        std::vector<void*> mapped;
    };
    std::map<uint32_t, StorageBufferData> storageBuffers; // key = binding
    
    // Internal methods
    void createUniformBuffer(uint32_t binding, VkDeviceSize bufferSize);
    void createStorageBuffer(uint32_t binding, VkDeviceSize bufferSize);
    void createDescriptorSetLayout();
    void createDescriptorPool();
    void allocateDescriptorSets();
    void updateDescriptorSets();
    void cleanup();
};

}

#endif