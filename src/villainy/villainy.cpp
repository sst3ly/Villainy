#include "villainy.hpp"

#include <vector>

#include "context.hpp"
#include "window.hpp"


namespace vlny{

void cleanup(Window* windows, int windowCount, Context& context){
    for(int i = 0; i < windowCount; i++){
        if(windows[i].swapchain.has_value()){
            vkDeviceWaitIdle(context.logicalDevice);
            windows[i].swapchain->cleanup();
            windows[i].swapchain.reset();
        }
    }

    if(context.transientCommandPool.has_value()){
        context.transientCommandPool.reset();
    }

    vkDestroyDevice(context.logicalDevice, nullptr);

    for(int i = 0; i < windowCount; i++){
        vkDestroySurfaceKHR(context.vkInstance, windows[i].windowSurface, nullptr);
    }

    if(context.config.enableValidationLayers){
        context.DestroyDebugUtilsMessengerEXT(context.debugMessenger, nullptr);
    }
    
    vkDestroyInstance(context.vkInstance, nullptr);
    for(int i = 0; i < windowCount; i++){
        glfwDestroyWindow(windows[i].window);
    }
    VILLAINY_VERBOSE_LOG(context.logger, "Cleanup complete!");
}

void cleanup(Window& window, Context& context){
    if(window.swapchain.has_value()){
        vkDeviceWaitIdle(context.logicalDevice);
        window.swapchain->cleanup();
        window.swapchain.reset();
    }

    if(context.transientCommandPool.has_value()){
        context.transientCommandPool.reset();
    }

    vkDestroyDevice(context.logicalDevice, nullptr);

    vkDestroySurfaceKHR(context.vkInstance, window.windowSurface, nullptr);
    
    if(context.config.enableValidationLayers){
        context.DestroyDebugUtilsMessengerEXT(context.debugMessenger, nullptr);
    }
    
    vkDestroyInstance(context.vkInstance, nullptr);
    glfwDestroyWindow(window.window);
    VILLAINY_VERBOSE_LOG(context.logger, "Cleanup complete!");
}

}
