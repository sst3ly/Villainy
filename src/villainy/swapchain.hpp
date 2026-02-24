#ifndef VILLAINY_SWAPCHAIN
#define VILLAINY_SWAPCHAIN

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <limits>
#include <stdexcept>
#include <optional>
#include <algorithm>

#include "renderpass.hpp"

namespace vlny{

class Context;
class Window;
class GraphicsPipeline;
template<typename Vertex> class Renderer;

class Swapchain{
public:
    Swapchain(Context& context, Window& window);

    void cleanup();
    VkExtent2D getExtent();
    VkFormat getImageFormat();
private:
    Context& context;
    Window& window;
    std::optional<RenderPass> renderPass;

    VkSwapchainKHR vkSwapchain;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;

    bool framebufferResized = false;

    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> swapchainFramebuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    void init();
    
    void createVkSwapchain();
    void createImageViews();
    void createRenderPass();
    void createFramebuffers();
    void createSyncObjects();

    void recreateSwapchain();
    void cleanupSwapchain();

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    friend class Context;
    friend class Window;
    friend class GraphicsPipeline;
    template<typename Vertex> friend class Renderer;
};

}

#endif