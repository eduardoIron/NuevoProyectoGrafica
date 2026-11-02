#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D atlas;
uniform vec4 color;
void main()
{
    float a = texture(atlas, vUV).r;
    FragColor = vec4(color.rgb, color.a * a);
}
