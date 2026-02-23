#ifndef VILLAINY_RENDER
#define VILLAINY_RENDER

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <stdexcept>
#include <string>
#include <utility>

#include "shader.hpp"
#include "command.hpp"
#include "buffer.hpp"
#include "renderpass.hpp"
#include "swapchain.hpp"
#include "window.hpp"

namespace vlny{

//class Window;
class Context;
//class Swapchain;

struct ColorVertex{
    glm::vec2 pos;
    glm::vec3 col;
};

struct VertexData{
    uint32_t binding = 0;
    VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    uint32_t stride = sizeof(ColorVertex);
    std::vector<std::pair<VkFormat, uint32_t>> vertexAttributes = {
        {VK_FORMAT_R32G32_SFLOAT, offsetof(ColorVertex, pos)},
        {VK_FORMAT_R32G32B32_SFLOAT, offsetof(ColorVertex, col)}
    };
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    bool primitiveRestart = false;
};

struct GraphicsPipelineConfig{
    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VertexData vertexData;
    // rasterizer
    bool depthClamp = false;
    VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
    float lineWidth = 1.0f;
    VkCullModeFlagBits cullMode = VK_CULL_MODE_BACK_BIT;
    VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
};

class GraphicsPipeline{
public:
    GraphicsPipeline(GraphicsPipelineConfig config, Context& context, Swapchain& swapchain, ShaderProgram& shaderProgram);
    GraphicsPipeline(Swapchain& swapchain, Context& context, ShaderProgram& shaderProgram);
    ~GraphicsPipeline();
private:
    GraphicsPipelineConfig config;
    Context& context;
    Swapchain& swapchain;
    ShaderProgram& shaderProgram;

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;

    void init();
    
    template<typename Vertex> friend class Renderer;
};

template<typename Vertex>
struct RenderObject{
    VertexBuffer<Vertex>& vb;
    IndexBuffer& ib;
    UniformBuffer& ub;
};

template<typename Vertex>
class Renderer{
public:
    Renderer(Context& context, Window& window, GraphicsPipeline& pipeline, Swapchain& swapchain);
    
    void drawFrame();

    int addRenderObject(RenderObject<Vertex> ro);
    int addRenderObject(VertexBuffer<Vertex>& vb, IndexBuffer& ib, UniformBuffer& ub);
    int addRenderObjects(std::vector<RenderObject<Vertex>> ros);
    RenderObject<Vertex> getRenderObject(int index);
    void removeRenderObject(int index);
private:
    std::vector<RenderObject<Vertex>> renderObjects;

    uint32_t currentFrame = 0;

    Context& context;
    Window& window;
    GraphicsPipeline& pipeline;
    Swapchain& swapchain;

    CommandPool commandPool;
    std::vector<CommandBuffer> cmdBufs;

    void recordCommandBuffer(CommandBuffer cmdBuf, uint32_t imageIndex);
};

}

#include "render.ipp"

#endif
