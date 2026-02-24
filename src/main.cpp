#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <chrono>

#include "villainy/villainy.hpp"
#include "villainy/render.hpp"
#include "villainy/shader.hpp"
#include "villainy/buffer.hpp"

// demo application
int main(){
    srand(time(NULL));

    if(!glfwInit()){
        throw std::runtime_error("Failed to init glfw!");
    }

    try{
        vlny::ContextConfig cfg;
        cfg.enableValidationLayers = true;
        cfg.minLogSeverity = vlny::LogSeverity::VERBOSE;
        vlny::Context context(cfg);
        vlny::Window window(context);

        {
            struct DummyUbo {
                alignas(16) float data[4] = {0.f, 0.f, 0.f, 0.f};
            };

            std::vector<vlny::ColorVertex> vertices = {
                {{ 0.0f, -0.6f}, {1.0f, 0.0f, 0.0f}}, // red
                {{ 0.6f,  0.6f}, {0.0f, 1.0f, 0.0f}}, // green
                {{-0.6f,  0.6f}, {0.0f, 0.0f, 1.0f}}  // blue
            };
            std::vector<vlny::ColorVertex> baseVertices = vertices;
            const float maxOffset = 0.1f;

            std::vector<uint16_t> indices = {0, 1, 2};

            std::vector<vlny::ShaderLoadInfo> shaderInfos = {
                {VK_SHADER_STAGE_VERTEX_BIT, "shaders/rainbow_triangle.vert.spv"},
                {VK_SHADER_STAGE_FRAGMENT_BIT, "shaders/rainbow_triangle.frag.spv"}
            };
            std::vector<vlny::ShaderLayoutDescriptor> shaderLayout = {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT}
            };

            vlny::ShaderProgram shaderProgram(context, shaderInfos, shaderLayout);
            vlny::GraphicsPipelineConfig pipelineCfg;
            pipelineCfg.cullMode = VK_CULL_MODE_NONE;
            vlny::GraphicsPipeline pipeline(pipelineCfg, context, window.getSwapchain(), shaderProgram);

            vlny::VertexBuffer<vlny::ColorVertex> vertexBuffer(context, vertices);
            vlny::IndexBuffer indexBuffer(context, indices);

            VkDescriptorPoolSize poolSize{};
            poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            poolSize.descriptorCount = static_cast<uint32_t>(window.getConfig().maxFramesInFlight);

            vlny::UniformBuffer uniformBuffer(context, window.getConfig(), sizeof(DummyUbo), {poolSize});
            uniformBuffer.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
            uniformBuffer.createDescriptorSetLayout();
            uniformBuffer.allocateDescriptorSets();
            uniformBuffer.updateDescriptorSets();

            DummyUbo dummy{};
            for(uint32_t i = 0; i < static_cast<uint32_t>(window.getConfig().maxFramesInFlight); ++i){
                uniformBuffer.updateBuffer(i, &dummy);
            }

            vlny::Renderer<vlny::ColorVertex> renderer(context, window, pipeline, window.getSwapchain());
            renderer.addRenderObject(vertexBuffer, indexBuffer, uniformBuffer);
        
            int frames = 0;
            auto startTime = std::chrono::high_resolution_clock::now();
            auto motionStart = std::chrono::high_resolution_clock::now();
            while(window.windowOpen()){
                glfwPollEvents();
                
                auto now = std::chrono::high_resolution_clock::now();
                float t = std::chrono::duration<float>(now - motionStart).count() * 2.0f;

                // animate vertices
                for(size_t i = 0; i < vertices.size(); ++i){
                    float phase = static_cast<float>(i) * 2.0f;
                    vertices[i].pos[0] =
                        baseVertices[i].pos[0] +
                        std::sin(t + phase) * maxOffset;

                    vertices[i].pos[1] =
                        baseVertices[i].pos[1] +
                        std::cos(t + phase) * maxOffset;
                }
                vertexBuffer.updateBuffer(vertices.data(), vertices.size() * sizeof(vlny::ColorVertex));
                
                // draw frame
                renderer.drawFrame();

                // fps logging
                float elapsed = std::chrono::duration<float, std::chrono::seconds::period>(now - startTime).count();
                if(elapsed >= 1.0f){
                    float fps = frames / elapsed;
                    context.logger.log(vlny::LogSeverity::INFO, "FPS: " + std::to_string(fps));
                    frames = 0;
                    startTime = now;
                }
                frames++;
            }

            context.waitIdle();
        }

        vlny::cleanup(window, context);
    }
    catch(const std::exception& e){
        std::cerr << "Error: " << e.what() << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwTerminate();
    return 0;
}
