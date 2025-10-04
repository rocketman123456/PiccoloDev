#pragma once
#include "runtime/function/render/gpu_pipeline.h"
#include "runtime/function/render/gpu_shader.h"
#include "runtime/function/render/utils/gpu_utils.h"

#include <volk.h>
// #include <vulkan/vulkan.h>

#include <memory>
#include <string>
#include <vector>

namespace Piccolo
{
    class GPUPipelineManager
    {
    public:
        GPUPipelineManager();
        ~GPUPipelineManager();

    private:
        std::vector<std::shared_ptr<GPUPipeline>> m_pipelines;
    };
} // namespace Piccolo
