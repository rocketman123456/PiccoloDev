#pragma once
#include "runtime/function/render/utils/gpu_shader_utils.h"

#include <volk.h>
// #include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <vector>

namespace Piccolo
{
    enum ShaderType : uint32_t
    {
        VertexShader,
        FragmanetShader,
    };

    class GPUShader
    {
    public:
        GPUShader(const std::string& shader_path, ShaderType type);
        ~GPUShader();

    private:
        std::string m_path;
        ShaderType  m_type;

        std::vector<uint32_t> m_spirv;

        VkShaderModule m_shader;
    };
} // namespace Piccolo
