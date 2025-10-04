#pragma once
#include "runtime/function/render/gpu_shader.h"
#include "runtime/function/render/utils/gpu_utils.h"

#include <volk.h>
// #include <vulkan/vulkan.h>

#include <memory>
#include <string>
#include <vector>

namespace Piccolo
{
    // 着色器阶段配置
    struct GPUShaderStageConfig
    {
        ShaderType  type;
        std::string shader_path;
        std::string entry_point = "main";

        std::vector<std::string> defines;
    };

    // 管道配置结构
    struct GPUPipelineConfig
    {
        std::string name;
        std::string description;

        std::vector<GPUShaderStageConfig> shader_stages;

        // VertexInputConfig                  vertex_input;
        // InputAssemblyConfig                input_assembly;
        // RasterizationConfig                rasterization;
        // MultisampleConfig                  multisample;
        // DepthStencilConfig                 depth_stencil;
        // ColorBlendConfig                   color_blend;
        // DynamicStateConfig                 dynamic_state;
        // std::map<std::string, std::string> parameters;
    };

    class GPUPipeline
    {
    public:
        GPUPipeline(VkDevice device, const GPUPipelineConfig& config);
        ~GPUPipeline();

    private:
        void createShaders();
        void createGraphicsPipeline();

        // TODO : add pipeline resource
        std::vector<std::string> m_shader_paths;
        std::vector<ShaderType>  m_types;
        std::vector<std::string> m_entry_points;

        std::vector<std::shared_ptr<GPUShader>> m_shaders;

        VkDevice         m_device;
        VkPipeline       m_pipeline;
        VkPipelineLayout m_pipeline_layout;
    };
} // namespace Piccolo