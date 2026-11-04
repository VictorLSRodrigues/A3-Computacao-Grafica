
#include "VAO.h"

VAO::VAO()
{
    glGenVertexArrays(1, &ID);
}

VAO::~VAO()
{
    Delete();
}

void VAO::Bind() const
{
    glBindVertexArray(ID);
}

void VAO::Unbind() const
{
    glBindVertexArray(0);
}

void VAO::Delete()
{
    if (ID != 0)
    {
        glDeleteVertexArrays(1, &ID);
        ID = 0;
    }
}
