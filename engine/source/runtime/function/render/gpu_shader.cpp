#include "runtime/function/render/gpu_shader.h"
#include "runtime/function/render/utils/gpu_shader_utils.h"

#include "runtime/core/base/macro.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Piccolo
{
    GPUShader::GPUShader(VkDevice device, const std::string& shader_path, ShaderType type, std::string entry_point)
    {
        m_device      = device;
        m_path        = shader_path;
        m_type        = type;
        m_entry_point = entry_point;

        LOG_DEBUG("shader path: {}", shader_path);

        std::string error;

        EShLanguage stage;
        switch (m_type)
        {
            case VertexShader:
                stage = EShLangVertex;
                break;
            case FragmanetShader:
                stage = EShLangFragment;
                break;
            case GeometryShader:
                stage = EShLangGeometry;
                break;
            case ComputeShader:
                stage = EShLangCompute;
                break;
            case MeshShader:
                stage = EShLangMesh;
                break;
            case TessellaionControlShader:
                stage = EShLangTessControl;
                break;
            case TessellaionEvaluationShader:
                stage = EShLangTessEvaluation;
                break;
        }

        shader::compile_glsl(shader_path, stage, m_spirv, error);

        LOG_DEBUG("compile glsl log: {}", error);

        m_shader = shader::create_shader_module(m_device, m_spirv);
    }

    GPUShader::~GPUShader()
    {
        LOG_DEBUG("clear shader {} resource", m_path);

        m_spirv.clear();

        vkDestroyShaderModule(m_device, m_shader, nullptr);
    }

    VkShaderModule GPUShader::getShaderModule() const { return m_shader; }

    VkPipelineShaderStageCreateInfo GPUShader::getCreateInfo()
    {
        VkShaderStageFlagBits stage;
        switch (m_type)
        {
            case VertexShader:
                stage = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case FragmanetShader:
                stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case GeometryShader:
                stage = VK_SHADER_STAGE_GEOMETRY_BIT;
                break;
            case ComputeShader:
                stage = VK_SHADER_STAGE_COMPUTE_BIT;
                break;
            case MeshShader:
                stage = VK_SHADER_STAGE_MESH_BIT_EXT;
                break;
            case TessellaionControlShader:
                stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
                break;
            case TessellaionEvaluationShader:
                stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                break;
        }

        VkPipelineShaderStageCreateInfo shader_stage_info {};
        shader_stage_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stage_info.stage  = stage;
        shader_stage_info.module = m_shader;
        shader_stage_info.pName  = m_entry_point.c_str();

        return shader_stage_info;
    }
} // namespace Piccolo