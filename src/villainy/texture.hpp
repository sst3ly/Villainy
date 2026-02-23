#ifndef VILLAINY_TEXTURE
#define VILLAINY_TEXTURE

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <stdexcept>

#include <stb/stb_image.h>

namespace vlny{

class Context;

struct SamplerConfig{
    VkFilter filter = VK_FILTER_LINEAR; // use VK_FILTER_NEAREST for pixel art
    VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; // tile when needed
    VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    bool anisotropy = true;
    bool compareEnable = false; // color comparison, used for shadow maps usually
    VkCompareOp compareOp = VK_COMPARE_OP_ALWAYS;
};  

class Sampler{
public:
    Sampler(SamplerConfig config, Context& context);
private:
    SamplerConfig config;
    Context& context;

    VkSampler vkSampler;

    void init();

    friend class DescriptorManager;
};

class Texture{
public:
    Texture(Context& context, std::string imagepath);
    ~Texture();
    void cleanup();
private:
    bool cleaned = false;
    Context& context;

    const char* imagepath;
    int texWidth, texHeight, texChannels;

    VkImage image;
    VkImageView imageView;
    VkDeviceMemory imageMemory;

    void init();

    friend class DescriptorManager;
};

void transitionImageLayout(Context& context, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
VkImageView makeImageView(Context& context, VkImage image, VkFormat format);

}

#endif