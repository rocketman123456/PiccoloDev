#include "runtime/function/render/gpu_shader.h"
#include "runtime/function/render/utils/gpu_shader_utils.h"

#include "runtime/core/base/macro.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Piccolo
{
    GPUShader::GPUShader(VkDevice device, const std::string& shader_path, ShaderType type)
    {
        m_device = device;
        m_path   = shader_path;
        m_type   = type;

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

        LOG_INFO("compile glsl log: {}", error);

        m_shader = shader::create_shader_module(m_device, m_spirv);
    }

    GPUShader::~GPUShader()
    {
        m_spirv.clear();

        vkDestroyShaderModule(m_device, m_shader, nullptr);
    }

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
        shader_stage_info.stage  = VK_SHADER_STAGE_VERTEX_BIT; // VK_SHADER_STAGE_FRAGMENT_BIT
        shader_stage_info.module = m_shader;
        shader_stage_info.pName  = "main";

        return shader_stage_info;
    }
} // namespace Piccolo