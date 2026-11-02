#version 330 core
// ============================================================================
//  Fragment shader principal
//  - Iluminacion Blinn-Phong: luz direccional (Sol/Luna) + 4 reflectores (spot) + 3 puntuales
//  - Sombras con mapa de profundidad + PCF 3x3
//  - Normal mapping sin tangentes (marco de cotangentes por derivadas)
//  - Decal de lineas de la cancha (segunda textura proyectada en el plano XZ)
//  - Niebla, tone mapping ACES y correccion gamma
// ============================================================================
in vec3 vWorld;
in vec3 vNormal;
in vec2 vUV;
in vec3 vColor;

out vec4 FragColor;

uniform vec3 viewPos;

// material
uniform vec3  matColor;
uniform float matSpec;
uniform float matShin;
uniform vec3  matEmissive;
uniform float selfIllum;
uniform float matAlpha;
uniform bool  useTex;
uniform bool  useNormalMap;
uniform float normalStrength;
uniform bool  alphaTest;
uniform bool  useDecal;
uniform vec2  decalOrigin;
uniform vec2  decalSize;
uniform vec3  decalOutsideTint;
uniform sampler2D texDiffuse;
uniform sampler2D texNormal;
uniform sampler2D texDecal;

// luces
uniform vec3  sunDir;        // hacia la luz principal
uniform vec3  sunColor;
uniform vec3  ambSky;
uniform vec3  ambGround;
uniform vec3  fogColor;
uniform float fogDensity;
uniform float exposure;
uniform int   nSpots;
uniform vec3  spotPos[4];
uniform vec3  spotDir[4];
uniform vec3  spotColor[4];
uniform vec2  spotCone[4];   // x = cos(interior), y = cos(exterior)
uniform int   nPoints;
uniform vec3  pointPos[3];
uniform vec3  pointColor[3];

// sombras
uniform sampler2DShadow shadowMap;
uniform mat4  lightSpace;
uniform bool  shadowsOn;
uniform float shadowTexel;

vec3 toLinear(vec3 c) { return pow(c, vec3(2.2)); }

// Normal mapping: construye la base tangente a partir de derivadas de pantalla
vec3 perturbNormal(vec3 N, vec3 p, vec2 uv, vec3 mapN)
{
    vec3 dp1 = dFdx(p), dp2 = dFdy(p);
    vec2 duv1 = dFdx(uv), duv2 = dFdy(uv);
    vec3 dp2perp = cross(dp2, N);
    vec3 dp1perp = cross(N, dp1);
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
    float invmax = inversesqrt(max(max(dot(T, T), dot(B, B)), 1e-12));
    mat3 TBN = mat3(T * invmax, B * invmax, N);
    return normalize(TBN * mapN);
}

float shadowFactor(vec3 worldPos, vec3 Ng, vec3 L)
{
    float cosT = clamp(dot(Ng, L), 0.0, 1.0);
    // desplazamiento a lo largo de la normal (mas fuerte con luz rasante)
    vec3 offsetPos = worldPos + Ng * (0.04 + 0.10 * (1.0 - cosT));
    vec4 ls = lightSpace * vec4(offsetPos, 1.0);
    vec3 p = ls.xyz / ls.w * 0.5 + 0.5;
    if (p.z > 1.0 || p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0) return 1.0;
    float bias = 0.00025;
    float sum = 0.0;
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            sum += texture(shadowMap, vec3(p.xy + vec2(x, y) * shadowTexel * 1.2, p.z - bias));
    return sum / 9.0;
}

// Blinn-Phong para una luz con direccion L (hacia la luz) y radiancia "rad"
vec3 lightTerm(vec3 L, vec3 rad, vec3 N, vec3 V, vec3 albedo)
{
    float ndl = max(dot(N, L), 0.0);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), matShin) * matSpec * (matShin + 8.0) * 0.0398 * ndl;
    return (albedo * ndl + vec3(spec)) * rad;
}

vec3 aces(vec3 x)
{
    x *= exposure;
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

void main()
{
    vec4 tex = useTex ? texture(texDiffuse, vUV) : vec4(1.0);
    float alpha = tex.a * matAlpha;
    if (alphaTest && alpha < 0.35) discard;
    if (alpha < 0.02) discard;

    vec3 albedo = toLinear(matColor) * toLinear(tex.rgb) * toLinear(vColor);

    // lineas de la cancha / color de la zona exterior
    if (useDecal)
    {
        vec2 duv = (vWorld.xz - decalOrigin) / decalSize;
        vec2 d = texture(texDecal, duv).rg;
        albedo *= mix(decalOutsideTint, vec3(1.0), d.g);
        albedo = mix(albedo, vec3(0.80, 0.80, 0.78), d.r * 0.95);
    }

    vec3 Ng = normalize(vNormal);
    if (!gl_FrontFacing) Ng = -Ng;
    vec3 N = Ng;
    if (useNormalMap)
    {
        vec3 m = texture(texNormal, vUV).xyz * 2.0 - 1.0;
        m.xy *= normalStrength;
        N = perturbNormal(Ng, vWorld, vUV, normalize(m));
    }
    vec3 V = normalize(viewPos - vWorld);

    // ambiente hemisferico
    vec3 col = albedo * mix(ambGround, ambSky, N.y * 0.5 + 0.5);

    // luz principal (Sol o Luna) con sombra
    vec3 L = normalize(sunDir);
    float sh = 1.0;
    if (shadowsOn && dot(Ng, L) > 0.0) sh = shadowFactor(vWorld, Ng, L);
    col += lightTerm(L, sunColor * sh, N, V, albedo);

    // reflectores del estadio
    for (int i = 0; i < nSpots; ++i)
    {
        vec3 Lv = spotPos[i] - vWorld;
        float dist = length(Lv);
        vec3 Ls = Lv / dist;
        float theta = dot(Ls, -spotDir[i]);
        float k = clamp((theta - spotCone[i].y) / (spotCone[i].x - spotCone[i].y), 0.0, 1.0);
        k = k * k * (3.0 - 2.0 * k);
        float att = 1.0 / (1.0 + 0.0015 * dist + 0.00035 * dist * dist);
        col += lightTerm(Ls, spotColor[i] * (k * att), N, V, albedo);
    }
    // luces puntuales (tunel, etc.)
    for (int i = 0; i < nPoints; ++i)
    {
        vec3 Lv = pointPos[i] - vWorld;
        float dist = length(Lv);
        vec3 Lp = Lv / dist;
        float att = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
        col += lightTerm(Lp, pointColor[i] * att, N, V, albedo);
    }

    col += matEmissive + albedo * selfIllum;

    // niebla exponencial
    float dview = length(viewPos - vWorld);
    float f = 1.0 - exp(-fogDensity * dview);
    col = mix(col, fogColor, clamp(f, 0.0, 1.0));

    col = aces(col);
    col = pow(col, vec3(1.0 / 2.2));
    FragColor = vec4(col, alpha);
}
