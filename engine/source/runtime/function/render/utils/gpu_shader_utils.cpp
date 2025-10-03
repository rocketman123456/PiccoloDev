#include "runtime/function/render/utils/gpu_shader_utils.h"

#include "runtime/core/base/macro.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/config_manager/config_manager.h"

#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>

#include <glslang/SPIRV/GlslangToSpv.h>
// #include <glslang/SPIRV/SpvBuilder.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace Piccolo
{
    namespace shader
    {
        Includer::IncludeResult* Includer::includeSystem(const char* headerName, const char*, size_t) { return includeLocal(headerName, nullptr, 0); }

        Includer::IncludeResult* Includer::includeLocal(const char* headerName, const char*, size_t)
        {
            std::string path;
            if (g_runtime_global_context.m_asset_manager)
            {
                path = g_runtime_global_context.m_asset_manager->getFullPath(path).string();
            }
            else
            {
                path = std::filesystem::path("asset/shader/include") / headerName;
            }

            std::ifstream file(path);
            if (!file.is_open())
                return nullptr;

            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            char*       content_copy = new char[content.size() + 1];
            std::copy(content.begin(), content.end(), content_copy);
            content_copy[content.size()] = '\0';

            return new IncludeResult(path, content_copy, content.size(), nullptr);
        }

        void Includer::releaseInclude(IncludeResult* result)
        {
            if (result != nullptr)
            {
                delete[] result->headerData;
                delete result;
            }
        }

        void start_compiler() { glslang::InitializeProcess(); }
        void stop_compiler() { glslang::FinalizeProcess(); }

        bool compile_glsl(const std::string& source_path, EShLanguage stage, std::vector<uint32_t>& spirvOut, std::string& error_log)
        {
            std::string path;
            if (g_runtime_global_context.m_asset_manager)
            {
                path = g_runtime_global_context.m_asset_manager->getFullPath(source_path).string();
            }
            else
            {
                path = std::filesystem::path(source_path);
            }

            std::ifstream file(path);
            if (!file.is_open())
            {
                error_log = "Failed to open shader file: " + path;
                return false;
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string shader_code = buffer.str();

            const char* shader_strings[1] = {shader_code.c_str()};

            glslang::TShader shader(stage);
            shader.setStrings(shader_strings, 1);
            shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
            shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
            shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);

            Includer    includer;
            EShMessages messages = EShMsgDefault;

            const TBuiltInResource* resources = GetDefaultResources();

            if (!shader.parse(resources, 100, false, messages, includer))
            {
                error_log = shader.getInfoLog();
                return false;
            }

            glslang::TProgram program;
            program.addShader(&shader);
            if (!program.link(messages))
            {
                error_log = program.getInfoLog();
                return false;
            }

            glslang::SpvOptions spv_options;
            spv_options.generateDebugInfo = true;

            glslang::GlslangToSpv(*program.getIntermediate(stage), spirvOut, &spv_options);
            return true;
        }

        VkShaderModule create_shader_module(VkDevice device, const std::vector<uint32_t>& code)
        {
            VkShaderModuleCreateInfo create_info {};
            create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            create_info.codeSize = code.size() * 4;
            create_info.pCode    = code.data();

            VkShaderModule shader_module;
            if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create shader module!");
            }

            return shader_module;
        }
    } // namespace shader
} // namespace Piccolo
