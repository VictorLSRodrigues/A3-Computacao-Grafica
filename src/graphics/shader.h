#pragma once

#include <glad/glad.h>
#include <string>
#include <glm/glm.hpp>

class Shader
{
private:
    GLuint program = 0;

public:
    GLuint ID = 0;

    Shader() = default;
    ~Shader() { destroy(); }

    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;

    bool create(const char *vertexSource, const char *fragmentSource);
    bool createFromFiles(const std::string &vertexPath, const std::string &fragmentPath);
    void use() const;
    void destroy();
    void setVec2(const char *name, float x, float y) const;
    void setVec3(const char *name, float x, float y, float z) const;
    void setVec4(const char *name, float x, float y, float z, float w) const;
    void setMat4(const char *name, const glm::mat4 &matrix) const;
    void setInt(const char *name, int value) const;
    void Default();
};