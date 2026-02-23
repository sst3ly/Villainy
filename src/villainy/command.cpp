#include "command.hpp"

#include "context.hpp"
#include "logger.hpp"

namespace vlny{

CommandPool::CommandPool(Context& context) : context(context){
    QueueFamilyIndices queueFamilyIndices = context.queueFamilyIndices;

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    /*VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded with new commands very often (may change memory allocation behavior)
    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be rerecorded individually, without this flag they all have to be reset together
    we use the reset bc we reset all of them each frame*/
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if(vkCreateCommandPool(context.logicalDevice, &poolInfo, nullptr, &vkCommandPool) != VK_SUCCESS){
        throw std::runtime_error("Failed to create command pool!");
    }
    VILLAINY_VERBOSE_LOG(context.logger, "Made command pool.");
}

CommandPool::~CommandPool(){
    if(vkCommandPool != VK_NULL_HANDLE){
        vkDestroyCommandPool(context.logicalDevice, vkCommandPool, nullptr);
        vkCommandPool = VK_NULL_HANDLE;
    }
}

std::vector<CommandBuffer> CommandPool::createCommandBuffers(Context& context, uint32_t n){
    std::vector<VkCommandBuffer> buffers(n);
    
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vkCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    /*VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for execution, but cannot be called from other command buffers.
    VK_COMMAND_BUFFER_LEVEL_SECONDARY: Cannot be submitted directly, but can be called from primary command buffers.*/
    allocInfo.commandBufferCount = (uint32_t) buffers.size();

    if(vkAllocateCommandBuffers(context.logicalDevice, &allocInfo, buffers.data()) != VK_SUCCESS){
        throw std::runtime_error("Failed to allocate command buffers!");
    }

    std::vector<CommandBuffer> cmdBufs;
    for(auto& buf : buffers){
        cmdBufs.emplace_back(context, *this, buf);
    }
    
    return cmdBufs;
}

// ------------------------------------------------------------------------------------------------------------------------

CommandBuffer::CommandBuffer(Context& context, CommandPool& commandPool) : context(context), commandPool(commandPool) {}   

void CommandBuffer::beginSingletimeCommands(){
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool.vkCommandPool;
    allocInfo.commandBufferCount = 1;

    vkAllocateCommandBuffers(context.logicalDevice, &allocInfo, &vkCommandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(vkCommandBuffer, &beginInfo);
}
void CommandBuffer::endSingletimeCommands(){
    vkEndCommandBuffer(vkCommandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkCommandBuffer;

    vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(context.graphicsQueue);

    vkFreeCommandBuffers(context.logicalDevice, commandPool.vkCommandPool, 1, &vkCommandBuffer);
}

CommandBuffer::CommandBuffer(Context& context, CommandPool& commandPool, VkCommandBuffer buf) : context(context), commandPool(commandPool), vkCommandBuffer(buf){}


}
