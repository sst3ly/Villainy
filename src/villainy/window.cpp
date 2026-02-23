#include "window.hpp"
#include "context.hpp"
#include "logger.hpp"

namespace vlny{

Window::Window(Context& context) : config(), context(context), logger(context.logger) { 
    init();
    swapchain.emplace(context, *this); // delayed construction(after init)
}
Window::Window(WindowConfig c, Context& context) : config(c), context(context), logger(context.logger) {
    init();
    swapchain.emplace(context, *this); // delayed construction(after init)
}

WindowConfig Window::getConfig(){
    return config;
}

void Window::setWindowSize(int width, int height){
    glfwSetWindowSize(window, width, height);
    config.width = width;
    config.height = height;
}
void Window::setWindowTitle(std::string name){
    glfwSetWindowTitle(window, name.c_str());
}
void Window::setWindowTitle(const char* name){
    glfwSetWindowTitle(window, name);
}

bool Window::windowOpen(){
    return !glfwWindowShouldClose(window);
}
Swapchain& Window::getSwapchain(){
    return swapchain.value();
}
void Window::setWindowShouldClose(){
    setWindowShouldClose(true);
}
void Window::setWindowShouldClose(bool val){
    glfwSetWindowShouldClose(window, val);
}

void Window::init(){
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, config.resizable);

    window = glfwCreateWindow(config.width, config.height, config.title.c_str(), nullptr, nullptr);
    if(!window){
        throw std::runtime_error("Failed to create GLFW window!");
    }

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    logger.log(VERBOSE, "Created glfw window.");
    //TODO: if(config.resizable){ glfwSetFramebufferSizeCallback(window, framebufferResizeCallback); }
    createSurface();
}

void Window::createSurface(){
    if(glfwCreateWindowSurface(context.vkInstance, window, nullptr, &windowSurface) != VK_SUCCESS){
        throw std::runtime_error("Failed to create window surface!");
    }
    logger.log(VERBOSE, "Created window surface!");
}

}
