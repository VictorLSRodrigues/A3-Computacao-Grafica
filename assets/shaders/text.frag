#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uAtlas; // sem setInt: o padrão é a unidade 0
uniform vec4 uColor;

void main()
{
    float coverage = texture(uAtlas, vUV).r; // o atlas tem um canal só: o quanto a letra cobre cada ponto
    FragColor = vec4(uColor.rgb, uColor.a * coverage);
}