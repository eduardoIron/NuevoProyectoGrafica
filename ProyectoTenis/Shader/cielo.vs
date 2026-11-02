#version 330 core
// Triangulo a pantalla completa (sin buffers): el cielo se calcula por pixel
out vec2 vNdc;
void main()
{
    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    vNdc = p * 2.0 - 1.0;
    gl_Position = vec4(vNdc, 1.0, 1.0);
}
