#include "swapchain.hpp"
#include "context.hpp"
#include "window.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "render.hpp"
#include "texture.hpp"

namespace vlny{

Swapchain::Swapchain(Context& context, Window& window) : context(context), window(window) {
    init();
}

void Swapchain::cleanup(){
    for(auto fence : inFlightFences){
        vkDestroyFence(context.logicalDevice, fence, nullptr);
    }

    for(auto semaphore : renderFinishedSemaphores){
        vkDestroySemaphore(context.logicalDevice, semaphore, nullptr);
    }

    for(auto semaphore : imageAvailableSemaphores){
        vkDestroySemaphore(context.logicalDevice, semaphore, nullptr);
    }

    for(auto framebuffer : swapchainFramebuffers){
        vkDestroyFramebuffer(context.logicalDevice, framebuffer, nullptr);
    }

    if(renderPass.has_value()){
        vkDestroyRenderPass(context.logicalDevice, renderPass->vkRenderPass, nullptr);
        renderPass.reset();
    }

    for(auto imageView : swapchainImageViews){
        vkDestroyImageView(context.logicalDevice, imageView, nullptr);
    }

    if(vkSwapchain != VK_NULL_HANDLE){
        vkDestroySwapchainKHR(context.logicalDevice, vkSwapchain, nullptr);
        vkSwapchain = VK_NULL_HANDLE;
    }
}

void Swapchain::init(){
    context.renderInit(window);
    createVkSwapchain();
    createRenderPass();
    createFramebuffers();
    createSyncObjects();
}
void Swapchain::createVkSwapchain(){
    SwapchainSupportDetails swapchainSupport = context.querySwapchainSupport(context.physicalDevice, window.windowSurface);
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapchainSupport.capabilities);

    swapchainImageFormat = surfaceFormat.format;
    swapchainExtent = extent;

    // get swapchain size
    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if(swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount){
        imageCount = swapchainSupport.capabilities.maxImageCount; // set to max if there is a max (max > 0)
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = window.windowSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    // TODO: add more configs for imageArrayLayers and imageUsage
    // >1 for stereoscopic applications
    createInfo.imageArrayLayers = 1; 
    // use VK_IMAGE_USAGE_TRANSFER_DST_BIT for post-processing (render to mem img, post process, mem img transfers to surface)
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; 

    QueueFamilyIndices indices = context.queueFamilyIndices;
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    
    if(indices.graphicsFamily != indices.presentFamily){
        // different queue families
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else{
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapchainSupport.capabilities.currentTransform; // no image rotations or flips

    // ignore other window blending
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE; // don't worry about pixels that are covered (ex by another window)

    // only one swapchain (assumption, may need another if the main one breaks)
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if(vkCreateSwapchainKHR(context.logicalDevice, &createInfo, nullptr, &vkSwapchain) != VK_SUCCESS){
        throw std::runtime_error("Failed to create swap chain!");
    }
    VILLAINY_VERBOSE_LOG(context.logger, "Created swapchain.");

    vkGetSwapchainImagesKHR(context.logicalDevice, vkSwapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(context.logicalDevice, vkSwapchain, &imageCount, swapchainImages.data());
    VILLAINY_VERBOSE_LOG(context.logger, "Created swapchain images.");

    createImageViews();
}
void Swapchain::createImageViews(){
    swapchainImageViews.resize(swapchainImages.size());
    for(size_t i = 0; i < swapchainImages.size(); i++){
        swapchainImageViews[i] = makeImageView(context, swapchainImages[i], swapchainImageFormat);
    }
    VILLAINY_VERBOSE_LOG(context.logger, "Made swapchain image views.");
}
void Swapchain::createRenderPass(){
    renderPass.emplace(swapchainImageFormat, context.logicalDevice);
    VILLAINY_VERBOSE_LOG(context.logger, "Made render pass.");
}
void Swapchain::createFramebuffers(){
    swapchainFramebuffers.resize(swapchainImageViews.size());
    for(int i = 0; i < swapchainFramebuffers.size(); i++){
        VkImageView attachments[] = {
            swapchainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        if(!renderPass.has_value()){
            throw std::runtime_error("Failed to access render pass!");
        }
        framebufferInfo.renderPass = renderPass->vkRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapchainExtent.width;
        framebufferInfo.height = swapchainExtent.height;
        framebufferInfo.layers = 1;

        if(vkCreateFramebuffer(context.logicalDevice, &framebufferInfo, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS){
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
    VILLAINY_VERBOSE_LOG(context.logger, "Made framebuffers.");
}
void Swapchain::createSyncObjects(){
    imageAvailableSemaphores.resize(context.config.maxFramesInFlight);
    renderFinishedSemaphores.resize(context.config.maxFramesInFlight);
    inFlightFences.resize(context.config.maxFramesInFlight);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    fenceInfo.pNext = nullptr;

    for(size_t i = 0; i < context.config.maxFramesInFlight; i++){
        if(vkCreateSemaphore(context.logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(context.logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(context.logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS){
            throw std::runtime_error("Failed to create semaphore/fence for a frame!");
        }
    }
}

void Swapchain::recreateSwapchain(){
    int width = 0, height = 0;
    glfwGetFramebufferSize(window.window, &width, &height);

    // pause while minimized
    if(context.config.pauseOnMinimize){
        while(width == 0 || height == 0){
            glfwGetFramebufferSize(window.window, &width, &height);
            glfwWaitEvents();
        }
    }

    vkDeviceWaitIdle(context.logicalDevice);

    cleanupSwapchain();

    createVkSwapchain();
    createFramebuffers();
}

void Swapchain::cleanupSwapchain(){
    for(auto framebuffer : swapchainFramebuffers){
        vkDestroyFramebuffer(context.logicalDevice, framebuffer, nullptr);
    }

    for(auto imageView : swapchainImageViews){
        vkDestroyImageView(context.logicalDevice, imageView, nullptr);
    }

    vkDestroySwapchainKHR(context.logicalDevice, vkSwapchain, nullptr);
}

// ------------------------------------------------------------------------------------------------------------------

VkSurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats){
    for(const auto& availableFormat : availableFormats){
        if(availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
            return availableFormat;
        }
    }

    // fallback to first
    // TODO: consider more advanced format rating system
    return availableFormats[0];
}
VkPresentModeKHR Swapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes){
    for(const auto& availablePresentMode : availablePresentModes){
        if(availablePresentMode == context.config.preferredSwapchainImagePresentMode){
            return availablePresentMode;
        }
    }
    // only one guaranteed to be available
    // TODO: consider letting the programmer rank preferred methods
    return VK_PRESENT_MODE_FIFO_KHR; 
}
VkExtent2D Swapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities){
    // screen coords match pixels
    if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()){
        return capabilities.currentExtent;
    } 
    // screen coords don't match pixels (calculate it + clamp to capabilities)
    else{
        int width, height;
        glfwGetFramebufferSize(window.window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

}
