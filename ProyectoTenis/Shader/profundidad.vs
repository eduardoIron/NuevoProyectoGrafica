#version 330 core
// Pasada de sombras: solo profundidad desde el punto de vista de la luz
layout(location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 lightSpace;
void main()
{
    gl_Position = lightSpace * model * vec4(aPos, 1.0);
}
