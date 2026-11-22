#version 330 core

out vec4 FragColor;

uniform vec4 lightColor;
uniform bool uFramebufferSRGB;

vec3 srgbToLinear(vec3 color)
{
    vec3 low = color / 12.92;
    vec3 high = pow((color + 0.055) / 1.055, vec3(2.4));
    return mix(high, low, lessThanEqual(color, vec3(0.04045)));
}

vec3 linearToSrgb(vec3 color)
{
    color = max(color, vec3(0.0));
    vec3 low = color * 12.92;
    vec3 high = 1.055 * pow(color, vec3(1.0 / 2.4)) - 0.055;
    return mix(high, low, lessThanEqual(color, vec3(0.0031308)));
}

void main()
{
    vec3 linearColor = srgbToLinear(lightColor.rgb);
    vec3 outputColor = uFramebufferSRGB ? linearColor : linearToSrgb(linearColor);
    FragColor = vec4(outputColor, lightColor.a);
}