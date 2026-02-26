#include "villainy.hpp"
#include "buffer.hpp"
#include "vulkan/vulkan_core.h"

#include <GLFW/glfw3.h>

namespace vlny{

bool QueueFamilyIndices::isComplete(){
    return graphicsFamily.has_value() && presentFamily.has_value();
}

Context::Context(void) : logger(config.appName) { 
    baseInit();
}
Context::Context(ContextConfig c) : config(c), logger(c.appName) { 
    logger.setMinSeverity(c.minLogSeverity);
    baseInit();
}

CommandPool& Context::getTransientCommandPool(){
    return transientCommandPool.value();
}

void Context::waitIdle(){
    if(logicalDevice != VK_NULL_HANDLE){
        vkDeviceWaitIdle(logicalDevice);
    }
}

Context& Context::operator=(Context&& other) {
    if (this == &other) return *this;

    waitIdle();

    config = std::move(other.config);
    logger = std::move(other.logger);

    // move vulkan handles
    vkInstance = other.vkInstance;
    physicalDevice = other.physicalDevice;
    logicalDevice = other.logicalDevice;
    graphicsQueue = other.graphicsQueue;
    presentQueue = other.presentQueue;
    debugMessenger = other.debugMessenger;
    queueFamilyIndices = other.queueFamilyIndices;
    maxAnisotropy = other.maxAnisotropy;

    // null out the other so its destructor doesn't double-destroy
    other.vkInstance = VK_NULL_HANDLE;
    other.physicalDevice = VK_NULL_HANDLE;
    other.logicalDevice = VK_NULL_HANDLE;
    other.debugMessenger = VK_NULL_HANDLE;

    baseInit();

    return *this;
}


// ---------------------------------------------------------------------------------------------------------

void Context::baseInit(){
    VILLAINY_VERBOSE_LOG(logger, "Initializing context...");
    createInstance();
    if(config.enableValidationLayers){
        setupDebugManager();
    }
    VILLAINY_VERBOSE_LOG(logger, "Context initialized");
}
void Context::renderInit(Window& window){
    selectPhysicalDevice(window.windowSurface);
    makeLogicalDevice(queueFamilyIndices);
    transientCommandPool.emplace(*this);
}

void Context::createInstance(){
    if(config.enableValidationLayers && !checkValidationLayerSupport()){
        throw std::runtime_error("Validation layers enabled, but not supported!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = config.appName.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(config.appVersionMajor, config.appVersionMinor, config.appVersionPatch);
    appInfo.pEngineName = config.engineName.c_str();
    appInfo.engineVersion = VK_MAKE_VERSION(config.engineVersionMajor, config.engineVersionMinor, config.engineVersionPatch);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    std::vector<const char*> extensions = getRequiredExtensions();
    extensions.insert(extensions.end(), config.vulkanExts.begin(), config.vulkanExts.end());

    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    
    VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
    if(config.enableValidationLayers){
        instanceInfo.enabledLayerCount = scast_ui32(config.validationLayers.size());
        instanceInfo.ppEnabledLayerNames = config.validationLayers.data();

        populateDebugMessengerInfo(debugInfo);
        debugInfo.pfnUserCallback = config.validationLayerDebugCallback;
        
        instanceInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugInfo;
    }
    else{
        instanceInfo.enabledLayerCount = 0;
        instanceInfo.pNext = nullptr;
    }

    if(config.macosDriverCompat){
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        instanceInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }
    instanceInfo.enabledExtensionCount = scast_ui32(extensions.size());
    instanceInfo.ppEnabledExtensionNames = extensions.data();

    if(vkCreateInstance(&instanceInfo, nullptr, &vkInstance) != VK_SUCCESS){
        throw std::runtime_error("Failed to create vulkan instance!");
    }
    VILLAINY_VERBOSE_LOG(logger, "Created VK Instance!");
}
void Context::setupDebugManager(){
    VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
    populateDebugMessengerInfo(debugInfo);
    debugInfo.pfnUserCallback = config.validationLayerDebugCallback;

    if (CreateDebugUtilsMessengerEXT(&debugInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("Failed to set up debug messenger!");
    }
    VILLAINY_VERBOSE_LOG(logger, "Made main debug messenger");
}

// ---------------------------------------------------------------------------------------------------------------

void Context::selectPhysicalDevice(VkSurfaceKHR surface){
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);
    
    if(deviceCount == 0){
        throw std::runtime_error("Failed to find a GPU with vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data());

    std::multimap<int, VkPhysicalDevice> candidates;
    for(const auto& device : devices){
        int score = ratePhysicalDevice(device, surface);
        candidates.insert(std::make_pair(score, device));
    }

    if(candidates.rbegin()->first > 0){
        physicalDevice = candidates.rbegin()->second;
        queueFamilyIndices = findQueueFamilies(physicalDevice, surface);
        VILLAINY_VERBOSE_LOG(logger, "Selected GPU.");
    }
    else{
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}
void Context::makeLogicalDevice(QueueFamilyIndices indices){
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    float queuePriority = 1.0f;
    for(uint32_t queueFamily : uniqueQueueFamilies){
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures supportedFeatures{};
    vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

    VkPhysicalDeviceFeatures deviceFeatures{};
    if(supportedFeatures.samplerAnisotropy){
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    }
    
    VkDeviceCreateInfo deviceCreateInfo{};

    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = scast_ui32(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = scast_ui32(config.deviceExts.size());
    deviceCreateInfo.ppEnabledExtensionNames = config.deviceExts.data();

    if(config.enableValidationLayers){
        deviceCreateInfo.enabledLayerCount = scast_ui32(config.validationLayers.size());
        deviceCreateInfo.ppEnabledLayerNames = config.validationLayers.data();
    }
    else{
        deviceCreateInfo.enabledLayerCount = 0;
    }

    if(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice) != VK_SUCCESS){
        throw std::runtime_error("Failed to create logical device!");
    }

    vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue);
}

// ------------------------------------------------------------------------------------------------------

bool Context::checkValidationLayerSupport(){
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : config.validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

std::vector<const char*> Context::getRequiredExtensions(){
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if(config.enableValidationLayers){
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;   
}
int Context::ratePhysicalDevice(VkPhysicalDevice device, VkSurfaceKHR surface){
    int score = 0;

    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU){
        score += 1000;
    }
    score += deviceProperties.limits.maxImageDimension2D;

    if(deviceFeatures.geometryShader){
        score += 100;
    }
    
    QueueFamilyIndices indices = findQueueFamilies(device, surface);

    bool swapChainSupported = false;
    bool extensionsSupported = deviceSupportsExtensions(device);
    if(extensionsSupported){
        SwapchainSupportDetails swapchainSupport = querySwapchainSupport(device, surface);
        swapChainSupported = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
    }
    
    if(!(indices.isComplete() && extensionsSupported && swapChainSupported)){ return 0; }

    /*
    bool anisotropySupported = deviceFeatures.samplerAnisotropy;

    if(!(indices.isComplete() && extensionsSupported && swapChainSupported && anisotropySupported)){
        return 0;
    }*/

    return score;
}
bool Context::deviceSupportsExtensions(VkPhysicalDevice device){
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(config.deviceExts.begin(), config.deviceExts.end());

    for(const auto& extension : availableExtensions){
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

QueueFamilyIndices Context::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface){
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for(const auto& queueFamily : queueFamilies){
        if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT){
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if(presentSupport){
            indices.presentFamily = i;
        }

        if(indices.isComplete()){
            break;
        }
        i++;
    }
    return indices;
}
SwapchainSupportDetails Context::querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface){
    SwapchainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if(formatCount != 0){
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    
    if(presentModeCount != 0){
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }
    
    return details;
}

VkResult Context::CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger){
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkInstance, "vkCreateDebugUtilsMessengerEXT");
    if(func != nullptr){
        return func(vkInstance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else{
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
void Context::populateDebugMessengerInfo(VkDebugUtilsMessengerCreateInfoEXT &debugInfo){
    debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
}
void Context::DestroyDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator){
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkInstance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(vkInstance, debugMessenger, pAllocator);
    }
}

}
