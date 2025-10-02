#pragma once
#include <glslang/Public/ShaderLang.h>

#include <string>
#include <vector>

namespace Piccolo
{
    class ShaderCompiler
    {
    public:
        ShaderCompiler();
        ~ShaderCompiler();

        bool compileGLSL(const std::string& sourcePath, EShLanguage stage, std::vector<uint32_t>& spirvOut, std::string& errorLog);

    private:
        class Includer : public glslang::TShader::Includer
        {
        public:
            IncludeResult* includeSystem(const char* headerName, const char* includerName, size_t) override;
            IncludeResult* includeLocal(const char* headerName, const char* includerName, size_t) override;
            void           releaseInclude(IncludeResult* result) override;
        };
    };
} // namespace Piccolo
