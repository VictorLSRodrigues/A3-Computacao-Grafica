#version 330 core

in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uIcon;
uniform vec4 uColor;

vec3 srgbToLinear(vec3 color)
{
    vec3 low = color / 12.92;
    vec3 high = pow((color + 0.055) / 1.055, vec3(2.4));
    return mix(high, low, lessThanEqual(color, vec3(0.04045)));
}

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
        srgbToLinear(uColor.rgb),
        uColor.a * alpha
    );
}