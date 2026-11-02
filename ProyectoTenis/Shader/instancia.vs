#version 330 core
// Vertex shader del PUBLICO: una sola malla de espectador dibujada N veces (instanciado)
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec4 aColor;       // a: 1 = camisa (tinte), 0.5 = piel, 0 = color fijo
layout(location = 4) in vec4 iPosS;        // xyz = posicion del asiento, w = coordenada a lo largo del perimetro
layout(location = 5) in vec4 iColYaw;      // rgb = color de camisa, w = giro (rad) hacia la cancha

uniform mat4 view;
uniform mat4 projection;
uniform float time;
uniform float cheer;       // 0..1 intensidad de festejo (el publico "salta")
uniform float waveOn;      // 0..1 "ola" activa
uniform float waveTime;

out vec3 vWorld;
out vec3 vNormal;
out vec2 vUV;
out vec3 vColor;

float hash(vec2 p) { return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453); }

void main()
{
    float seed = hash(iPosS.xz + iPosS.yy);
    vec3 col;
    if (aColor.a > 0.9)       col = iColYaw.rgb;
    else if (aColor.a > 0.4)  col = mix(vec3(0.96, 0.78, 0.64), vec3(0.42, 0.28, 0.18), hash(iPosS.zx * 1.7));
    else                      col = aColor.rgb;
    vColor = col;

    vec3 p = aPos;
    // festejo individual
    float bob = cheer * abs(sin(time * (5.0 + 3.0 * seed) + seed * 20.0)) * 0.12;
    // "ola": se ponen de pie al pasar la onda
    float w = waveOn * smoothstep(0.55, 1.0, sin(iPosS.w * 0.30 - waveTime * 2.4));
    p.y += bob + w * 0.38;
    p.y *= 1.0 + w * 0.10;

    float c = cos(iColYaw.w), s = sin(iColYaw.w);
    vec3 pr = vec3(p.x * c + p.z * s, p.y, -p.x * s + p.z * c);
    vec3 nr = vec3(aNormal.x * c + aNormal.z * s, aNormal.y, -aNormal.x * s + aNormal.z * c);

    vec3 world = pr + iPosS.xyz;
    vWorld = world;
    vNormal = nr;
    vUV = aUV;
    gl_Position = projection * view * vec4(world, 1.0);
}
