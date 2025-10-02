#include "runtime/function/render/utils/gpu_shader_utils.h"

using namespace Piccolo;

#include <iostream>

int main()
{
    ShaderCompiler compiler;

    std::vector<uint32_t> spirv;
    std::string           error;

    if (compiler.compileGLSL("asset/shader/glsl/_shader_base.vert", EShLangVertex, spirv, error))
    {
        std::cout << "Shader compiled to SPIR-V, size: " << spirv.size() << " words\n";
    }
    else
    {
        std::cerr << "Shader compilation failed:\n" << error << "\n";
    }
}
