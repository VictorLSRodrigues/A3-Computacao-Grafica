#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uAtlas; // O atlas é associado à unidade de textura 0.
uniform vec4 uColor;
uniform bool uFramebufferSRGB;

vec3 srgbToLinear(vec3 color)
{
    vec3 low = color / 12.92;
    vec3 high = pow((color + 0.055) / 1.055, vec3(2.4));
    return mix(high, low, lessThanEqual(color, vec3(0.04045)));
}

void main()
{
    float coverage = texture(uAtlas, vUV).r; // Intensidade do glifo no atlas, usada como transparência.
    FragColor = vec4(uFramebufferSRGB ? srgbToLinear(uColor.rgb) : uColor.rgb, uColor.a * coverage);
}