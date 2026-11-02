#version 330 core
// Vertex shader principal de la escena (geometria estatica y dinamica)
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec4 aColor;

uniform mat4 model;
uniform mat3 normalMat;      // transpose(inverse(mat3(model))) calculada en CPU
uniform mat4 view;
uniform mat4 projection;
uniform vec2 uvScale;

out vec3 vWorld;
out vec3 vNormal;
out vec2 vUV;
out vec3 vColor;

void main()
{
    vec4 w = model * vec4(aPos, 1.0);
    vWorld = w.xyz;
    vNormal = normalMat * aNormal;
    vUV = aUV * uvScale;
    vColor = aColor.rgb;
    gl_Position = projection * view * w;
}
