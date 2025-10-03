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
    class GPUPipeline
    {
    public:
        GPUPipeline(VkDevice device);
        ~GPUPipeline();

    private:
        void createShaders();
        void createGraphicsPipeline();

        // TODO : add pipeline resource
        std::vector<std::string> m_shader_paths;
        std::vector<ShaderType>  m_types;

        std::shared_ptr<GPUShader> m_shaders;

        VkDevice   m_device;
        VkPipeline m_pipeline;
    };
} // namespace Piccolo