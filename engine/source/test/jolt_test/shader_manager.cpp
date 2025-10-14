#include "shader_manager.h"
#include <glad/glad.h>
#include <iostream>

using namespace std;

ShaderManager::ShaderManager()
    : m_shader_program(0)
{
}

ShaderManager::~ShaderManager()
{
    Shutdown();
}

bool ShaderManager::Initialize()
{
    m_shader_program = CreateShaderProgram();
    if (m_shader_program == 0)
    {
        cerr << "着色器程序创建失败" << endl;
        return false;
    }
    
    cout << "着色器管理器初始化成功" << endl;
    return true;
}

void ShaderManager::Shutdown()
{
    if (m_shader_program != 0)
    {
        glDeleteProgram(m_shader_program);
        m_shader_program = 0;
    }
}

GLuint ShaderManager::CompileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, nullptr, info_log);
        cerr << "着色器编译失败: " << info_log << endl;
    }

    return shader;
}

GLuint ShaderManager::CreateShaderProgram()
{
    const char* vertex_shader_source = GetVertexShaderSource();
    const char* fragment_shader_source = GetFragmentShaderSource();

    GLuint vertex_shader = CompileShader(GL_VERTEX_SHADER, vertex_shader_source);
    GLuint fragment_shader = CompileShader(GL_FRAGMENT_SHADER, fragment_shader_source);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char info_log[512];
        glGetProgramInfoLog(program, 512, nullptr, info_log);
        cerr << "着色器程序链接失败: " << info_log << endl;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return program;
}

const char* ShaderManager::GetVertexShaderSource() const
{
    return R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoord;
        
        out vec3 FragPos;
        out vec3 Normal;
        out vec2 TexCoord;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        void main()
        {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            TexCoord = aTexCoord;
            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )";
}

const char* ShaderManager::GetFragmentShaderSource() const
{
    return R"(
        #version 330 core
        out vec4 FragColor;
        
        in vec3 FragPos;
        in vec3 Normal;
        in vec2 TexCoord;
        
        uniform vec3 lightPos;
        uniform vec3 viewPos;
        uniform vec3 objectColor;
        uniform bool useTexture;
        uniform sampler2D textureSampler;
        uniform bool useAlpha;
        uniform float alpha;
        
        void main()
        {
            vec3 color = objectColor;
            if (useTexture)
            {
                color = texture(textureSampler, TexCoord).rgb;
            }
            
            if (useAlpha)
            {
                FragColor = vec4(color, alpha);
                return;
            }
            
            // 环境光
            float ambientStrength = 0.3;
            vec3 ambient = ambientStrength * color;
            
            // 漫反射
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * color;
            
            // 镜面反射
            float specularStrength = 0.5;
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            vec3 specular = specularStrength * spec * vec3(1.0);
            
            vec3 result = ambient + diffuse + specular;
            FragColor = vec4(result, 1.0);
        }
    )";
}
