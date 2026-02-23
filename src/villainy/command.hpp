#ifndef VILLAINY_COMMAND
#define VILLAINY_COMMAND

#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace vlny{

class Context;
class CommandBuffer;

struct CommandPool{
    CommandPool(Context& context);
    ~CommandPool();

    Context& context;
    VkCommandPool vkCommandPool = VK_NULL_HANDLE;

    std::vector<CommandBuffer> createCommandBuffers(Context& context, uint32_t n);
};

class CommandBuffer{
public:
    CommandBuffer(Context& context, CommandPool& commandPool);
    CommandBuffer(Context& context, CommandPool& commandPool, VkCommandBuffer buf);

    void beginSingletimeCommands();
    void endSingletimeCommands();
private:
    Context& context;
    CommandPool& commandPool;

    VkCommandBuffer vkCommandBuffer;

    friend struct CommandPool;
    template <typename Vertex> friend class Renderer;
    friend void copyBufferToImage(Context& context, CommandPool commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    friend void transitionImageLayout(Context& context, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    friend void copyBuffer(Context& context, VkBuffer srcBuf, VkBuffer dstBuf, VkDeviceSize size);
};

}

#endif
