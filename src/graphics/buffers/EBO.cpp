
#include "EBO.h"

EBO::EBO()
{
    glGenBuffers(1, &ID);
}

EBO::~EBO()
{
    Delete();
}

void EBO::Bind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
}

void EBO::Upload(
    const std::vector<unsigned int>& indices,
    GLenum usage
)
{
    Bind();

    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            indices.size() * sizeof(unsigned int)
        ),
        indices.data(),
        usage
    );

    indexCount = static_cast<GLsizei>(indices.size());
}

void EBO::Delete()
{
    if (ID != 0)
    {
        glDeleteBuffers(1, &ID);
        ID = 0;
        indexCount = 0;
    }
}
