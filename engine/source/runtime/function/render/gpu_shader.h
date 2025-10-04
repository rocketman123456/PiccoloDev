#pragma once

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
        GeometryShader,
        ComputeShader,
        MeshShader,
        TessellaionControlShader,
        TessellaionEvaluationShader,
    };

    class GPUShader
    {
    public:
        GPUShader(VkDevice device, const std::string& shader_path, ShaderType type, std::string entry_point);
        ~GPUShader();

        VkShaderModule                  getShaderModule() const;
        VkPipelineShaderStageCreateInfo getCreateInfo();

    private:
        std::string m_path;
        ShaderType  m_type;
        std::string m_entry_point;

        std::vector<uint32_t> m_spirv;

        VkDevice       m_device;
        VkShaderModule m_shader;
    };
} // namespace Piccolo
