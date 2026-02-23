#ifndef VILLAINY_SHADER
#define VILLAINY_SHADER

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <stdexcept>
#include <vector>

#include "utils.hpp"

namespace vlny{

class Context;
class GraphicsPipeline;

struct ShaderLoadInfo{
    VkShaderStageFlagBits stage;
    std::string filepath;
    std::string entrypoint = "main";
    VkSpecializationInfo* VkSpecializationInfo = nullptr; // shader constants
};

struct ShaderLayoutDescriptor{
    uint32_t binding;
    VkDescriptorType descriptorType;
    VkShaderStageFlagBits stage;
    uint32_t descriptorCount = 1;
};

class ShaderProgram{
public:
    ShaderProgram(Context& context, std::vector<ShaderLoadInfo> shaderInfos, std::vector<ShaderLayoutDescriptor> descriptorSetLayout);
    ~ShaderProgram();

private:
    int numShaders;
    VkDevice device;
    std::vector<VkShaderModule> vkShaderModules;
    std::vector<VkPipelineShaderStageCreateInfo> vkShaderStages;
    std::vector<std::string> shaderEntrypoints;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

    VkShaderModule makeVkShaderModule(ShaderLoadInfo shaderInfo);

    friend class GraphicsPipeline;
};

}

#endif
