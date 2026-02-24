#include "shader.hpp"

#include "context.hpp"

namespace vlny{

ShaderProgram::ShaderProgram(Context& context, std::vector<ShaderLoadInfo> shaderInfos, std::vector<ShaderLayoutDescriptor> descriptorSetLayoutData) : device(context.logicalDevice){
    numShaders = shaderInfos.size();
    vkShaderModules.resize(numShaders);
    vkShaderStages.resize(numShaders);
    shaderEntrypoints.resize(numShaders);
    for(int i = 0; i < numShaders; i++){
        vkShaderModules[i] = makeVkShaderModule(shaderInfos[i]);
        shaderEntrypoints[i] = shaderInfos[i].entrypoint;

        vkShaderStages[i] = {};
        vkShaderStages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vkShaderStages[i].stage = shaderInfos[i].stage;
        vkShaderStages[i].module = vkShaderModules[i];
        vkShaderStages[i].pName = shaderEntrypoints[i].c_str();
        vkShaderStages[i].pSpecializationInfo = shaderInfos[i].vkSpecializationInfo;
    }
    VILLAINY_VERBOSE_LOG(context.logger, "Created shader modules & stages.");

    // make descriptor set layout
    int numDescriptorLayouts = descriptorSetLayoutData.size();
    std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings(numDescriptorLayouts);
    for(int i = 0; i < numDescriptorLayouts; i++){
        descriptorSetLayoutBindings[i].binding = descriptorSetLayoutData[i].binding;
        descriptorSetLayoutBindings[i].descriptorCount = descriptorSetLayoutData[i].descriptorCount;
        descriptorSetLayoutBindings[i].descriptorType = descriptorSetLayoutData[i].descriptorType;
        descriptorSetLayoutBindings[i].pImmutableSamplers = nullptr;
        descriptorSetLayoutBindings[i].stageFlags = descriptorSetLayoutData[i].stage;
    }

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = scast_ui32(numDescriptorLayouts);
    layoutCreateInfo.pBindings = descriptorSetLayoutBindings.data();

    if(vkCreateDescriptorSetLayout(context.logicalDevice, &layoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS){
        throw std::runtime_error("Failed to create descriptor set layout!");
    }

    VILLAINY_VERBOSE_LOG(context.logger, "Created descriptor set layout.");
}

ShaderProgram::~ShaderProgram(){
    if(descriptorSetLayout != VK_NULL_HANDLE){
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        descriptorSetLayout = VK_NULL_HANDLE;
    }
    for(auto shaderModule : vkShaderModules){
        if(shaderModule != VK_NULL_HANDLE){
            vkDestroyShaderModule(device, shaderModule, nullptr);
        }
    }
}

VkShaderModule ShaderProgram::makeVkShaderModule(ShaderLoadInfo shaderInfo){
    auto shaderCode = readFile(shaderInfo.filepath);
    
    VkShaderModuleCreateInfo shaderCreateInfo{};
    shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderCreateInfo.codeSize = shaderCode.size();
    shaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

    VkShaderModule vkShaderModule;
    if(vkCreateShaderModule(device, &shaderCreateInfo, nullptr, &vkShaderModule) != VK_SUCCESS){
        throw std::runtime_error("Failed to create shader module!");
    }

    return vkShaderModule;
}

}
