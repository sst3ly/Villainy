#ifndef VILLAINY_WINDOW_HPP
#define VILLAINY_WINDOW_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <optional>
#include <stdexcept>

#include "swapchain.hpp"

namespace vlny{

class Context;
class Logger;
class GraphicsPipeline;

struct WindowConfig{
    int width = 800;
    int height = 600;
    std::string title = "Villainy";
    bool fullscreen = false;
    bool resizable = true;

    double clearColor[3] = {0.0f, 0.0f, 0.0f};
};

class Window{
public:
    Window(Context& context);
    Window(WindowConfig c, Context& context);

    WindowConfig getConfig();

    void setWindowSize(int width, int height);
    void setWindowTitle(std::string name);
    void setWindowTitle(const char* name);

    bool windowOpen();
    void setWindowShouldClose();
    void setWindowShouldClose(bool val);
    Swapchain& getSwapchain();
private:
    WindowConfig config;
    Context& context;
    Logger& logger;
    std::optional<Swapchain> swapchain;

    GLFWwindow* window;
    VkSurfaceKHR windowSurface;

    void init();
    void createSurface();

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height){
        auto win = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        win->logger.log(VERBOSE, "Window resized to " + std::to_string(width) + "x" + std::to_string(height));
        win->config.width = width;
        win->config.height = height;
        if(win->context.config.pauseOnMinimize && (width == 0 || height == 0)){
            win->logger.log(VERBOSE, "Window minimized, pausing context...");
        }
        if(win->swapchain.has_value()){
            win->swapchain->framebufferResized = true;
        }
    }

    friend class Context;
    friend class Swapchain;
    friend class GraphicsPipeline;
    friend void cleanup(Window* windows, int windowCount, Context& context);
    friend void cleanup(Window& window, Context& context);
};

}

#endif
