#include "runtime/function/render/gpu_pipeline.h"

namespace Piccolo
{
    GPUPipeline::GPUPipeline(VkDevice device)
    {
        m_device = device;

        createShaders();
        createGraphicsPipeline();
    }

    GPUPipeline::~GPUPipeline()
    {
        //
    }

    void GPUPipeline::createShaders()
    {
        //
    }

    void GPUPipeline::createGraphicsPipeline()
    {
        // VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        // VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        // VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        // vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        // vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        // vertShaderStageInfo.module = vertShaderModule;
        // vertShaderStageInfo.pName = "main";

        // VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        // fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        // fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        // fragShaderStageInfo.module = fragShaderModule;
        // fragShaderStageInfo.pName = "main";

        // VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        // vkDestroyShaderModule(device, fragShaderModule, nullptr);
        // vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }
} // namespace Piccolo