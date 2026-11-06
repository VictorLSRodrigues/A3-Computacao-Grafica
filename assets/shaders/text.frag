#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uAtlas; // O atlas é associado à unidade de textura 0.
uniform vec4 uColor;

void main()
{
    float coverage = texture(uAtlas, vUV).r; // Intensidade do glifo no atlas, usada como transparência.
    FragColor = vec4(uColor.rgb, uColor.a * coverage);
}