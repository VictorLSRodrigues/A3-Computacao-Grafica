
#include "VBO.h"

VBO::VBO()
{
    glGenBuffers(1, &ID);
}

VBO::~VBO()
{
    Delete();
}

void VBO::Bind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, ID);
}

void VBO::Unbind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Enviar vértices antigos (vector<float>)
void VBO::Upload(
    const std::vector<float>& vertices,
    GLenum usage
)
{
    Bind();

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            vertices.size() * sizeof(float)
        ),
        vertices.data(),
        usage
    );

    Unbind();
}

// Enviar vértices estruturados (vector<Vertex>)
void VBO::Upload(
    const std::vector<Vertex>& vertices,
    GLenum usage
)
{
    Bind();

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            vertices.size() * sizeof(Vertex)
        ),
        vertices.data(),
        usage
    );

    Unbind();
}

void VBO::Delete()
{
    if (ID != 0)
    {
        glDeleteBuffers(1, &ID);
        ID = 0;
    }
}
