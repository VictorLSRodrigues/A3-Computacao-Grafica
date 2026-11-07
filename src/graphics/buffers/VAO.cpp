#include "VAO.h"

VAO::~VAO()
{
    Delete();
}

void VAO::ensureCreated() const
{
    if (ID == 0)
        glGenVertexArrays(1, &ID);
}

void VAO::Bind() const
{
    ensureCreated();
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