#include "texture.hpp"
#include "context.hpp"
#include "buffer.hpp"

namespace vlny{

Sampler::Sampler(SamplerConfig config, Context& context) : config(config), context(context) {
    init();
}

void Sampler::init(){
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = config.filter;
    samplerInfo.minFilter = config.filter;
    samplerInfo.addressModeU = config.addressModeU;
    samplerInfo.addressModeV = config.addressModeV;
    samplerInfo.addressModeW = config.addressModeW;
    samplerInfo.anisotropyEnable = VK_FALSE;
    if(config.anisotropy){
        if(context.maxAnisotropy > 0){
            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = context.maxAnisotropy;
        }
    }
    
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    // unnormalized: [0, width/height)  |  normalized: [0, 1)
    samplerInfo.compareEnable = config.compareEnable;
    samplerInfo.compareOp = config.compareOp;
    // TODO: make configs for the rest of these
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if(vkCreateSampler(context.logicalDevice, &samplerInfo, nullptr, &vkSampler) != VK_SUCCESS){
        throw std::runtime_error("Failed to create sampler!");
    }
    VILLAINY_VERBOSE_LOG(context.logger, "Made sampler.");
}

// ------------------------------------------------------------------------------------------------------------------------

Texture::Texture(Context& context, std::string imagepath) : context(context), imagepath(imagepath.c_str()) {
    init();    
}
Texture::~Texture(){
    if(!cleaned){ cleanup(); }
}
void Texture::cleanup(){
    cleaned = true;
    vkDestroyImageView(context.logicalDevice, imageView, nullptr);
    vkDestroyImage(context.logicalDevice, image, nullptr);
    vkFreeMemory(context.logicalDevice, imageMemory, nullptr);
}

void Texture::init(){
    stbi_uc* pixels = stbi_load(imagepath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if(!pixels){
        throw std::runtime_error(std::string("Failed to load image ") + std::string(imagepath));
    }
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(context, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    
    void* data;
    vkMapMemory(context.logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(context.logicalDevice, stagingBufferMemory);

    stbi_image_free(pixels);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = scast_ui32(texWidth);
    imageInfo.extent.height = scast_ui32(texHeight);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    /*VK_IMAGE_TILING_LINEAR: Texels are laid out in row-major order like our pixels array
    VK_IMAGE_TILING_OPTIMAL: Texels are laid out in an implementation defined order for optimal access*/
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    /*VK_IMAGE_LAYOUT_UNDEFINED: Not usable by the GPU and the very first transition will discard the texels.
    VK_IMAGE_LAYOUT_PREINITIALIZED: Not usable by the GPU, but the first transition will preserve the texels.*/
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT; // TODO: multisampling
    
    if(vkCreateImage(context.logicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS){
        throw std::runtime_error("Failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(context.logicalDevice, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(context.physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if(vkAllocateMemory(context.logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS){
        throw std::runtime_error("Failed to allocate image memory!");
    }

    vkBindImageMemory(context.logicalDevice, image, imageMemory, 0);

    transitionImageLayout(context, image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(context, context.getTransientCommandPool(), stagingBuffer, image, scast_ui32(texWidth), scast_ui32(texHeight));
    transitionImageLayout(context, image, VK_FORMAT_R8G8B8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(context.logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(context.logicalDevice, stagingBufferMemory, nullptr);

    VILLAINY_VERBOSE_LOG(context.logger, "Created image.");

    imageView = makeImageView(context, image, VK_FORMAT_R8G8B8A8_SRGB);
}

void transitionImageLayout(Context& context, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout){
    CommandBuffer cmdBuf(context, context.getTransientCommandPool());
    cmdBuf.beginSingletimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // means that we aren't transferring queue families
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // if we are, use the src/dst queue family indices
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    //barrier.srcAccessMask = 0;
    //barrier.dstAccessMask = 0;

    VkPipelineStageFlags srcStage;
    VkPipelineStageFlags dstStage;

    if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL){
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL){
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else{
        throw std::invalid_argument("Unsupported layout transition!");
    }
    
    vkCmdPipelineBarrier(cmdBuf.vkCommandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    cmdBuf.endSingletimeCommands();
}

VkImageView makeImageView(Context& context, VkImage image, VkFormat format){
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;

    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0; // TODO: mips & levels
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    
    if(vkCreateImageView(context.logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS){
        throw std::runtime_error("Failed to create image view!");
    }
    return imageView;
}

}