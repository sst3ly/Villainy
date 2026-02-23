#ifndef VILLAINY
#define VILLAINY

#include <vector>


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "context.hpp"
#include "window.hpp"

namespace vlny{

void cleanup(Window* windows, int windowCount, Context& context);
void cleanup(Window& window, Context& context);

}

#endif