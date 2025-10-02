#pragma once
#include <glslang/Public/ShaderLang.h>

#include <volk.h>
// #include <vulkan/vulkan.h>

#include <string>
#include <vector>

namespace Piccolo
{
    namespace shader
    {
        class Includer : public glslang::TShader::Includer
        {
        public:
            IncludeResult* includeSystem(const char* headerName, const char* includerName, size_t) override;
            IncludeResult* includeLocal(const char* headerName, const char* includerName, size_t) override;
            void           releaseInclude(IncludeResult* result) override;
        };

        void start_compiler();
        void stop_compiler();
        bool compile_glsl(const std::string& sourcePath, EShLanguage stage, std::vector<uint32_t>& spirvOut, std::string& errorLog);

        VkShaderModule create_shader_module(VkDevice device, const std::vector<uint32_t>& code);
    } // namespace shader
} // namespace Piccolo
