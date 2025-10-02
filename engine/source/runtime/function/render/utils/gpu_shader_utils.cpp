#include "runtime/function/render/utils/gpu_shader_utils.h"

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
    static TBuiltInResource InitResources()
    {
        TBuiltInResource Resources;

        Resources.maxLights                                 = 32;
        Resources.maxClipPlanes                             = 6;
        Resources.maxTextureUnits                           = 32;
        Resources.maxTextureCoords                          = 32;
        Resources.maxVertexAttribs                          = 64;
        Resources.maxVertexUniformComponents                = 4096;
        Resources.maxVaryingFloats                          = 64;
        Resources.maxVertexTextureImageUnits                = 32;
        Resources.maxCombinedTextureImageUnits              = 80;
        Resources.maxTextureImageUnits                      = 32;
        Resources.maxFragmentUniformComponents              = 4096;
        Resources.maxDrawBuffers                            = 32;
        Resources.maxVertexUniformVectors                   = 128;
        Resources.maxVaryingVectors                         = 8;
        Resources.maxFragmentUniformVectors                 = 16;
        Resources.maxVertexOutputVectors                    = 16;
        Resources.maxFragmentInputVectors                   = 15;
        Resources.minProgramTexelOffset                     = -8;
        Resources.maxProgramTexelOffset                     = 7;
        Resources.maxClipDistances                          = 8;
        Resources.maxComputeWorkGroupCountX                 = 65535;
        Resources.maxComputeWorkGroupCountY                 = 65535;
        Resources.maxComputeWorkGroupCountZ                 = 65535;
        Resources.maxComputeWorkGroupSizeX                  = 1024;
        Resources.maxComputeWorkGroupSizeY                  = 1024;
        Resources.maxComputeWorkGroupSizeZ                  = 64;
        Resources.maxComputeUniformComponents               = 1024;
        Resources.maxComputeTextureImageUnits               = 16;
        Resources.maxComputeImageUniforms                   = 8;
        Resources.maxComputeAtomicCounters                  = 8;
        Resources.maxComputeAtomicCounterBuffers            = 1;
        Resources.maxVaryingComponents                      = 60;
        Resources.maxVertexOutputComponents                 = 64;
        Resources.maxGeometryInputComponents                = 64;
        Resources.maxGeometryOutputComponents               = 128;
        Resources.maxFragmentInputComponents                = 128;
        Resources.maxImageUnits                             = 8;
        Resources.maxCombinedImageUnitsAndFragmentOutputs   = 8;
        Resources.maxCombinedShaderOutputResources          = 8;
        Resources.maxImageSamples                           = 0;
        Resources.maxVertexImageUniforms                    = 0;
        Resources.maxTessControlImageUniforms               = 0;
        Resources.maxTessEvaluationImageUniforms            = 0;
        Resources.maxGeometryImageUniforms                  = 0;
        Resources.maxFragmentImageUniforms                  = 8;
        Resources.maxCombinedImageUniforms                  = 8;
        Resources.maxGeometryTextureImageUnits              = 16;
        Resources.maxGeometryOutputVertices                 = 256;
        Resources.maxGeometryTotalOutputComponents          = 1024;
        Resources.maxGeometryUniformComponents              = 1024;
        Resources.maxGeometryVaryingComponents              = 64;
        Resources.maxTessControlInputComponents             = 128;
        Resources.maxTessControlOutputComponents            = 128;
        Resources.maxTessControlTextureImageUnits           = 16;
        Resources.maxTessControlUniformComponents           = 1024;
        Resources.maxTessControlTotalOutputComponents       = 4096;
        Resources.maxTessEvaluationInputComponents          = 128;
        Resources.maxTessEvaluationOutputComponents         = 128;
        Resources.maxTessEvaluationTextureImageUnits        = 16;
        Resources.maxTessEvaluationUniformComponents        = 1024;
        Resources.maxTessPatchComponents                    = 120;
        Resources.maxPatchVertices                          = 32;
        Resources.maxTessGenLevel                           = 64;
        Resources.maxViewports                              = 16;
        Resources.maxVertexAtomicCounters                   = 0;
        Resources.maxTessControlAtomicCounters              = 0;
        Resources.maxTessEvaluationAtomicCounters           = 0;
        Resources.maxGeometryAtomicCounters                 = 0;
        Resources.maxFragmentAtomicCounters                 = 8;
        Resources.maxCombinedAtomicCounters                 = 8;
        Resources.maxAtomicCounterBindings                  = 1;
        Resources.maxVertexAtomicCounterBuffers             = 0;
        Resources.maxTessControlAtomicCounterBuffers        = 0;
        Resources.maxTessEvaluationAtomicCounterBuffers     = 0;
        Resources.maxGeometryAtomicCounterBuffers           = 0;
        Resources.maxFragmentAtomicCounterBuffers           = 1;
        Resources.maxCombinedAtomicCounterBuffers           = 1;
        Resources.maxAtomicCounterBufferSize                = 16384;
        Resources.maxTransformFeedbackBuffers               = 4;
        Resources.maxTransformFeedbackInterleavedComponents = 64;
        Resources.maxCullDistances                          = 8;
        Resources.maxCombinedClipAndCullDistances           = 8;
        Resources.maxSamples                                = 4;
        Resources.maxMeshOutputVerticesNV                   = 256;
        Resources.maxMeshOutputPrimitivesNV                 = 512;
        Resources.maxMeshWorkGroupSizeX_NV                  = 32;
        Resources.maxMeshWorkGroupSizeY_NV                  = 1;
        Resources.maxMeshWorkGroupSizeZ_NV                  = 1;
        Resources.maxTaskWorkGroupSizeX_NV                  = 32;
        Resources.maxTaskWorkGroupSizeY_NV                  = 1;
        Resources.maxTaskWorkGroupSizeZ_NV                  = 1;
        Resources.maxMeshViewCountNV                        = 4;

        Resources.limits.nonInductiveForLoops                 = 1;
        Resources.limits.whileLoops                           = 1;
        Resources.limits.doWhileLoops                         = 1;
        Resources.limits.generalUniformIndexing               = 1;
        Resources.limits.generalAttributeMatrixVectorIndexing = 1;
        Resources.limits.generalVaryingIndexing               = 1;
        Resources.limits.generalSamplerIndexing               = 1;
        Resources.limits.generalVariableIndexing              = 1;
        Resources.limits.generalConstantMatrixVectorIndexing  = 1;

        return Resources;
    }

    ShaderCompiler::ShaderCompiler() { glslang::InitializeProcess(); }

    ShaderCompiler::~ShaderCompiler() { glslang::FinalizeProcess(); }

    bool ShaderCompiler::compileGLSL(const std::string& sourcePath, EShLanguage stage, std::vector<uint32_t>& spirvOut, std::string& errorLog)
    {
        std::ifstream file(sourcePath);
        if (!file.is_open())
        {
            errorLog = "Failed to open shader file: " + sourcePath;
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string shaderCode = buffer.str();

        const char* shaderStrings[1] = {shaderCode.c_str()};

        glslang::TShader shader(stage);
        shader.setStrings(shaderStrings, 1);
        shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
        shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
        shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);

        Includer    includer;
        EShMessages messages = EShMsgDefault;

        const TBuiltInResource* resources = GetDefaultResources();

        if (!shader.parse(resources, 100, false, messages, includer))
        {
            errorLog = shader.getInfoLog();
            return false;
        }

        glslang::TProgram program;
        program.addShader(&shader);
        if (!program.link(messages))
        {
            errorLog = program.getInfoLog();
            return false;
        }

        glslang::SpvOptions spvOptions;
        glslang::GlslangToSpv(*program.getIntermediate(stage), spirvOut, &spvOptions);
        return true;
    }

    // Include handler
    ShaderCompiler::Includer::IncludeResult* ShaderCompiler::Includer::includeSystem(const char* headerName, const char*, size_t)
    {
        return includeLocal(headerName, nullptr, 0);
    }

    ShaderCompiler::Includer::IncludeResult* ShaderCompiler::Includer::includeLocal(const char* headerName, const char*, size_t)
    {
        std::string path = std::filesystem::path("shaders/include") / headerName;

        // std::string full_path = g_runtime_global_context.m_asset_manager->getFullPath(path).string();

        std::ifstream file(path);
        if (!file.is_open())
            return nullptr;

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        char*       content_copy = new char[content.size() + 1];
        std::copy(content.begin(), content.end(), content_copy);
        content_copy[content.size()] = '\0';

        return new IncludeResult(path, content_copy, content.size(), content_copy);
    }

    void ShaderCompiler::Includer::releaseInclude(IncludeResult* result)
    {
        if (result != nullptr)
        {
            if (result->userData != nullptr)
            {
                delete[] result->userData;
            }
            delete result;
        }
    }
} // namespace Piccolo
