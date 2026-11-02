#version 330 core
// "Halo" aditivo alrededor de las lamparas de los reflectores (billboards)
uniform mat4 view;
uniform mat4 projection;
uniform vec3 centers[16];
uniform float size;
out vec2 vUV;
void main()
{
    vec2 corner = vec2(float(gl_VertexID & 1), float((gl_VertexID >> 1) & 1)) * 2.0 - 1.0;
    vUV = corner;
    vec3 right = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 up    = vec3(view[0][1], view[1][1], view[2][1]);
    vec3 p = centers[gl_InstanceID] + (right * corner.x + up * corner.y) * size;
    gl_Position = projection * view * vec4(p, 1.0);
}
