#version 330 core
// Cielo procedural: gradiente, crepusculo, Sol, Luna, estrellas y nubes
in vec2 vNdc;
out vec4 FragColor;

uniform mat4 invProj;
uniform mat3 invViewRot;
uniform vec3 sunDir;
uniform vec3 moonDir;
uniform float dayFactor;
uniform vec3 fogColor;
uniform float exposure;
uniform float time;

float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float noise(vec2 p)
{
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1, 0)), f.x), mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), f.x), f.y);
}
float fbm(vec2 p)
{
    float s = 0.0, a = 0.5;
    for (int i = 0; i < 5; ++i) { s += a * noise(p); p = p * 2.03 + 11.7; a *= 0.5; }
    return s;
}

void main()
{
    vec4 v = invProj * vec4(vNdc, 1.0, 1.0);
    vec3 dir = normalize(invViewRot * normalize(v.xyz / v.w));
    float h = clamp(dir.y, 0.0, 1.0);

    vec3 zenithDay = vec3(0.10, 0.28, 0.70), horizonDay = vec3(0.55, 0.72, 0.92);
    vec3 zenithNight = vec3(0.004, 0.008, 0.028), horizonNight = vec3(0.020, 0.035, 0.085);
    vec3 zenith = mix(zenithNight, zenithDay, dayFactor);
    vec3 horizon = mix(horizonNight, horizonDay, dayFactor);

    // crepusculo cerca del Sol cuando esta bajo
    float elev = sunDir.y;
    float tw = clamp(1.0 - abs(elev) / 0.35, 0.0, 1.0);
    float towardSun = pow(max(dot(normalize(vec3(dir.x, 0.0, dir.z) + 1e-4), normalize(vec3(sunDir.x, 0.0, sunDir.z) + 1e-4)), 0.0), 3.0);
    horizon = mix(horizon, vec3(1.0, 0.45, 0.18), tw * (0.35 + 0.65 * towardSun) * 0.9);
    vec3 sky = mix(horizon, zenith, pow(h, 0.55));

    // estrellas
    if (dayFactor < 0.5 && dir.y > 0.0)
    {
        vec2 sp = dir.xz / (dir.y + 0.25) * 90.0;
        float st = step(0.9965, hash(floor(sp)));
        float tw2 = 0.6 + 0.4 * sin(time * 3.0 + hash(floor(sp)) * 50.0);
        sky += vec3(st * tw2) * (1.0 - dayFactor * 2.0);
    }
    // Sol
    float sd = max(dot(dir, sunDir), 0.0);
    vec3 sunCol = mix(vec3(1.0, 0.55, 0.25), vec3(1.0, 0.95, 0.8), clamp(elev * 2.0, 0.0, 1.0));
    sky += sunCol * (pow(sd, 1500.0) * 12.0 + pow(sd, 18.0) * 0.30 + pow(sd, 4.0) * 0.06) * smoothstep(-0.12, 0.05, elev);
    // Luna
    float md = max(dot(dir, moonDir), 0.0);
    sky += vec3(0.85, 0.90, 1.0) * (smoothstep(0.9993, 0.9996, md) * 0.9 + pow(md, 60.0) * 0.07) * (1.0 - dayFactor);

    // nubes
    if (dir.y > 0.02)
    {
        vec2 cuv = dir.xz / (dir.y + 0.15) * 1.3 + vec2(time * 0.012, time * 0.004);
        float c = smoothstep(0.50, 0.78, fbm(cuv));
        vec3 cloudLit = mix(vec3(0.03, 0.04, 0.07), vec3(1.0, 0.98, 0.95), dayFactor);
        cloudLit = mix(cloudLit, vec3(1.0, 0.65, 0.45), tw * 0.5 * dayFactor);
        sky = mix(sky, cloudLit, c * 0.8 * smoothstep(0.02, 0.2, dir.y));
    }
    vec3 outc = pow(clamp(sky, 0.0, 1.0), vec3(1.0 / 2.2));
    // debajo del horizonte -> mismo color que la niebla de la escena (tone mapping + gamma)
    vec3 x = fogColor * exposure;
    vec3 fogDisp = pow(clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0), vec3(1.0 / 2.2));
    outc = mix(outc, fogDisp, smoothstep(0.03, -0.05, dir.y));

    FragColor = vec4(outc, 1.0);
}
