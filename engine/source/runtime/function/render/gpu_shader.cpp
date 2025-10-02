#include "runtime/function/render/gpu_shader.h"

namespace Piccolo
{
    GPUShader::GPUShader(const std::string& shader_path, ShaderType type)
    {
        m_path = shader_path;
        m_type = type;
    }

    GPUShader::~GPUShader() { m_spirv.clear(); }
} // namespace Piccolo