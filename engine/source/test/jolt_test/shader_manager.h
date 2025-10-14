#pragma once

#include "jolt_character_test_common.h"

// 着色器管理器
class ShaderManager
{
public:
    ShaderManager();
    ~ShaderManager();

    bool Initialize();
    void Shutdown();

    // 着色器程序访问
    GLuint GetShaderProgram() const { return m_shader_program; }

    // 着色器编译
    GLuint CompileShader(GLenum type, const char* source);
    GLuint CreateShaderProgram();

private:
    GLuint m_shader_program;

    // 着色器源码
    const char* GetVertexShaderSource() const;
    const char* GetFragmentShaderSource() const;
};
