#include "shader.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>

// Cria o programa de shader a partir dos dois textos (vertex e fragment).
// Devolve true se deu tudo certo e false se algo falhou.
bool Shader::criar(const char *vertexFonte, const char *fragmentFonte)
{
    limpar(); // se já existia um programa, apaga antes de criar outro

    GLint sucesso;
    char log[512];

    // 1. vertex shader
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexFonte, nullptr);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &sucesso);
    if (!sucesso)
    {
        glGetShaderInfoLog(vertex, 512, nullptr, log);
        std::cout << "Erro no vertex shader:\n" << log << std::endl;
        glDeleteShader(vertex);
        return false;
    }

    // 2. fragment shader
    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentFonte, nullptr);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &sucesso);
    if (!sucesso)
    {
        glGetShaderInfoLog(fragment, 512, nullptr, log);
        std::cout << "Erro no fragment shader:\n" << log << std::endl;
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return false;
    }

    // 3. juntar no programa e ligar
    programa = glCreateProgram();
    glAttachShader(programa, vertex);
    glAttachShader(programa, fragment);
    glLinkProgram(programa);
    glGetProgramiv(programa, GL_LINK_STATUS, &sucesso);

    // 4. os shaders soltos não são mais necessários (deu certo ou não)
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    if (!sucesso)
    {
        glGetProgramInfoLog(programa, 512, nullptr, log);
        std::cout << "Erro ao ligar o programa:\n" << log << std::endl;
        glDeleteProgram(programa);
        programa = 0;
        return false;
    }

    return true;
}

// Escolhe este shader para os próximos desenhos.
void Shader::usar() const
{
    glUseProgram(programa);
}

// Apaga o programa da placa de vídeo (chamar antes de fechar a janela).
void Shader::limpar()
{
    glDeleteProgram(programa);
    programa = 0;
}

// Lê um arquivo de texto inteiro para dentro de uma string.
// Devolve false se não conseguir abrir o arquivo.
static bool lerArquivo(const std::string &caminho, std::string &conteudo)
{
    std::ifstream arquivo(caminho);
    if (!arquivo.is_open())
    {
        std::cout << "Não consegui abrir o arquivo: " << caminho << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << arquivo.rdbuf();
    conteudo = buffer.str();
    return true;
}

// Lê o vertex e o fragment shader de arquivos e cria o programa.
bool Shader::criarDeArquivos(const std::string &caminhoVertex, const std::string &caminhoFragment)
{
    std::string vertexFonte, fragmentFonte;
    if (!lerArquivo(caminhoVertex, vertexFonte) || !lerArquivo(caminhoFragment, fragmentFonte))
        return false;

    return criar(vertexFonte.c_str(), fragmentFonte.c_str());
}

void Shader::setVec2(const char* nome, float x, float y) const
{
    glUniform2f(glGetUniformLocation(programa, nome), x, y);
}

void Shader::setVec4(const char* nome, float x, float y, float z, float w) const
{
    glUniform4f(glGetUniformLocation(programa, nome), x, y, z, w);
}

void Shader::setMat4(const char* nome, const glm::mat4& matriz) const
{
    GLint localizacao = glGetUniformLocation(programa, nome);

    glUniformMatrix4fv(
        localizacao,
        1,
        GL_FALSE,
        glm::value_ptr(matriz)
    );
}