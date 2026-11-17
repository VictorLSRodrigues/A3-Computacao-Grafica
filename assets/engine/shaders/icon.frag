#version 330 core

in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uIcon;
uniform vec4 uColor;

void main()
{
    vec4 icon = texture(uIcon, vUV);

    // Aceita PNG com transparência e imagens RGB com ícones brancos
    // sobre fundo preto.
    float brightness = max(
        icon.r,
        max(icon.g, icon.b)
    );

    float alpha = max(icon.a, brightness);

    FragColor = vec4(
        uColor.rgb,
        uColor.a * alpha
    );
}