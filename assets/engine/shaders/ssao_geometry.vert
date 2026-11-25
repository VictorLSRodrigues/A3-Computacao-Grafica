#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 3) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 viewNormal;

void main()
{
    mat4 viewModel = view * model;
    vec4 viewPosition = viewModel * vec4(aPos, 1.0);
    mat3 normalMatrix = transpose(inverse(mat3(viewModel)));
    vec3 transformedNormal = normalMatrix * aNormal;
    float normalLengthSquared = dot(transformedNormal, transformedNormal);
    viewNormal = normalLengthSquared > 0.00000001
        ? transformedNormal * inversesqrt(normalLengthSquared)
        : vec3(0.0);
    gl_Position = projection * viewPosition;
}
