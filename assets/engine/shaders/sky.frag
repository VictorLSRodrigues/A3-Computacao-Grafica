#version 330 core

out vec4 FragColor;

in vec3 texDir;

uniform samplerCube skybox;
uniform bool uFramebufferSRGB;

vec3 linearToSrgb(vec3 color)
{
    color = max(color, vec3(0.0));
    vec3 low = color * 12.92;
    vec3 high = 1.055 * pow(color, vec3(1.0 / 2.4)) - 0.055;
    return mix(high, low, lessThanEqual(color, vec3(0.0031308)));
}

void main()
{
    vec4 sampledColor = texture(skybox, texDir);
    vec3 outputColor = uFramebufferSRGB ? sampledColor.rgb : linearToSrgb(sampledColor.rgb);
    FragColor = vec4(outputColor, sampledColor.a);
}