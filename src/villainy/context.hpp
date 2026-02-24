#ifndef VILLAINY_CONTEXT
#define VILLAINY_CONTEXT

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <vector>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>
#include <optional>
#include <cstring>

#include "logger.hpp"
#include "utils.hpp"
#include "command.hpp"
//#include "buffer.hpp"

namespace vlny{

class Window;
template<typename Vertex>
struct VertexBuffer;
struct IndexBuffer;
struct UniformBuffer;
class ShaderProgram;
class GraphicsPipeline;
struct CommandPool;
class DescriptorManager;
template <typename Vertex>
class Renderer;

struct QueueFamilyIndices{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete();
};

struct SwapchainSupportDetails{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct ContextConfig{
    std::string appName = "villainy_app";
    int appVersionMajor = 0, appVersionMinor = 0, appVersionPatch = 0;
    std::string engineName = "villainy";
    int engineVersionMajor = 0, engineVersionMinor = 0, engineVersionPatch = 0;

    std::vector<const char*> vulkanExts = {};

    LogSeverity minLogSeverity = INFO;

    bool enableValidationLayers = false;
    std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
    VKAPI_ATTR VkBool32 VKAPI_CALL (*validationLayerDebugCallback)(VkDebugUtilsMessageSeverityFlagBitsEXT messegeSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) = Logger::validationLayerCallback;

    std::vector<const char*> deviceExts = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    bool macosDriverCompat = true;
};

class Context{
public:
    Context(void);
    Context(ContextConfig c);

    CommandPool& getTransientCommandPool();
    void waitIdle();

    Logger logger;
private:
    ContextConfig config;

    VkDebugUtilsMessengerEXT debugMessenger;

    QueueFamilyIndices queueFamilyIndices;

    VkInstance vkInstance;
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    int maxAnisotropy = -1;

    std::optional<CommandPool> transientCommandPool;

    void baseInit();
    void renderInit(Window& window);

    // baseInit submethods
    void createInstance();
    void setupDebugManager();

    // renderInit submethods
    void selectPhysicalDevice(VkSurfaceKHR surface);
    void makeLogicalDevice(QueueFamilyIndices qfi);

    // helper methods
    bool checkValidationLayerSupport();

    std::vector<const char*> getRequiredExtensions();
    int ratePhysicalDevice(VkPhysicalDevice device, VkSurfaceKHR surface);
    bool deviceSupportsExtensions(VkPhysicalDevice device);

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
    SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

    VkResult CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    void populateDebugMessengerInfo(VkDebugUtilsMessengerCreateInfoEXT &debugCreateInfo);
    void DestroyDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

    VkDevice getLogicalDevice() const { return logicalDevice; }

    // friends
    friend class Window;
    friend class Swapchain;
    friend class ShaderProgram;
    friend class GraphicsPipeline;
    friend struct CommandPool;
    friend class Texture;
    friend class CommandBuffer;
    friend class Sampler;
    template<typename Vertex> friend struct VertexBuffer;
    friend struct IndexBuffer;
    friend struct UniformBuffer;
    friend class DescriptorManager;
    template<typename Vertex> friend class Renderer;
    friend VkImageView makeImageView(Context& context, VkImage image, VkFormat format);
    friend void createBuffer(Context& context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    friend void cleanup(Window* windows, int windowCount, Context& context);
    friend void cleanup(Window& window, Context& context);
};

}
#endif
