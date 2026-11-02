#version 330 core
layout(location = 0) in vec4 aData;   // xy = posicion en pixeles, zw = uv
uniform vec2 screen;
out vec2 vUV;
void main()
{
    vUV = aData.zw;
    vec2 p = vec2(aData.x / screen.x * 2.0 - 1.0, 1.0 - aData.y / screen.y * 2.0);
    gl_Position = vec4(p, 0.0, 1.0);
}
