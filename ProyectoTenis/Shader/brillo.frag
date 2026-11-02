#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform vec3 color;
void main()
{
    float r2 = dot(vUV, vUV);
    if (r2 > 1.0) discard;
    float core = exp(-r2 * 14.0);
    float halo = exp(-r2 * 3.2) * 0.35;
    FragColor = vec4(color * (core + halo), 1.0);
}
