#ifndef VILLAINY_RENDERPASS
#define VILLAINY_RENDERPASS

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <stdexcept>
#include <string>
#include <utility>

#include "shader.hpp"
#include "command.hpp"
#include "buffer.hpp"

namespace vlny{

struct RenderPass{
    VkRenderPass vkRenderPass;

    RenderPass(VkFormat format, VkDevice device);
};

}

#endif