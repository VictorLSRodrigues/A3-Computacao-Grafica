#version 330 core

in vec3 viewNormal;
layout (location = 0) out vec3 normalOutput;

void main()
{
    normalOutput = normalize(viewNormal);
}
