#include "shader.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>

// Compila os shaders de vértice e fragmento e os combina em um programa.
bool Shader::create(const char *vertexSource, const char *fragmentSource)
{
    destroy(); // Descarta o programa anterior antes de substituí-lo.

    GLint sucesso;
    char log[512];

    // Compila o shader de vértice.
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexSource, nullptr);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &sucesso);
    if (!sucesso)
    {
        glGetShaderInfoLog(vertex, 512, nullptr, log);
        std::cout << "Erro no vertex shader:\n"
                  << log << std::endl;
        glDeleteShader(vertex);
        return false;
    }

    // Compila o shader de fragmento.
    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentSource, nullptr);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &sucesso);
    if (!sucesso)
    {
        glGetShaderInfoLog(fragment, 512, nullptr, log);
        std::cout << "Erro no fragment shader:\n"
                  << log << std::endl;
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return false;
    }

    // Vincula os dois shaders em um programa.
    program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &sucesso);

    // Após a vinculação, o programa mantém o código necessário.
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    if (!sucesso)
    {
        glGetProgramInfoLog(program, 512, nullptr, log);
        std::cout << "Erro ao ligar o programa:\n"
                  << log << std::endl;
        glDeleteProgram(program);
        program = 0;
        return false;
    }
    ID = program;
    return true;
}

void Shader::use() const
{
    glUseProgram(program);
}

// Libera o programa enquanto o contexto OpenGL ainda está ativo.
void Shader::destroy()
{
    glDeleteProgram(program);
    program = 0;
    ID = 0;
}

// Lê o conteúdo inteiro do arquivo; retorna false se ele não puder ser aberto.
static bool readFile(const std::string &path, std::string &content)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cout << "Não consegui abrir o arquivo: " << path << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    content = buffer.str();
    return true;
}

// Lê os dois arquivos e cria o programa de shader.
bool Shader::createFromFiles(const std::string &vertexPath, const std::string &fragmentPath)
{
    std::string vertexSource, fragmentSource;
    if (!readFile(vertexPath, vertexSource) || !readFile(fragmentPath, fragmentSource))
        return false;

    return create(vertexSource.c_str(), fragmentSource.c_str());
}

void Shader::setVec2(const char *name, float x, float y) const
{
    glUniform2f(glGetUniformLocation(program, name), x, y);
}

void Shader::setVec3(const char *name, float x, float y, float z) const
{
    glUniform3f(glGetUniformLocation(program, name), x, y, z);
}

void Shader::setVec4(const char *name, float x, float y, float z, float w) const
{
    glUniform4f(glGetUniformLocation(program, name), x, y, z, w);
}

void Shader::setMat4(const char *name, const glm::mat4 &matrix) const
{
    GLint location = glGetUniformLocation(program, name);

    glUniformMatrix4fv(
        location,
        1,
        GL_FALSE,
        glm::value_ptr(matrix));
}

void Shader::setInt(const char *name, int value) const
{
    glUniform1i(glGetUniformLocation(program, name), value);
}

void Shader::Default()
{
    createFromFiles(
        "shaders/default.vert",
        "shaders/default.frag");
}