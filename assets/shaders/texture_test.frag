#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uTexture; // sem setInt: o padrão é 0 = unidade 0

void main()
{
    FragColor = texture(uTexture, vUV);
}