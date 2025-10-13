#include "runtime/function/render/utils/gpu_shader_utils.h"

using namespace Piccolo;

#include <iostream>

int main()
{
    shader::start_compiler();

    std::vector<uint32_t> spirv;
    std::string           error;

    if (shader::compile_glsl("asset/shader/glsl/axis.vert", EShLangVertex, spirv, error))
    {
        std::cout << "Log: " << error << std::endl;
        std::cout << "Shader compiled to SPIR-V, size: " << spirv.size() << " words\n";
    }
    else
    {
        std::cout << "Log: " << error << std::endl;
        std::cerr << "Shader compilation failed:\n" << error << "\n";
    }

    if (shader::compile_glsl("asset/shader/glsl/axis.frag", EShLangFragment, spirv, error))
    {
        std::cout << "Log: " << error << std::endl;
        std::cout << "Shader compiled to SPIR-V, size: " << spirv.size() << " words\n";
    }
    else
    {
        std::cout << "Log: " << error << std::endl;
        std::cerr << "Shader compilation failed:\n" << error << "\n";
    }

    shader::stop_compiler();
}
