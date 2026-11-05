#version 330 core
// Iluminacion basica (Phong): luz direccional (sol/luna) + 4 reflectores (spot) + 1 luz puntual (tunel)

struct Material { float shininess; };

struct DirLight
{
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight
{
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    vec3 diffuse;
    float constant;
    float linear;
    float quadratic;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 color;

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform SpotLight spotLights[4];
uniform int numSpots;
uniform vec3 pointPos;
uniform vec3 pointColor;
uniform Material material;
uniform sampler2D texture_diffuse1;
uniform float specStrength;
uniform float opacity;
uniform int useTexture = 1;
uniform vec4 objectColor = vec4(1.0);

void main()
{
        vec4 texColor = (useTexture == 1) ? texture(texture_diffuse1, TexCoords) : objectColor;
    if (texColor.a < 0.1)
        discard;

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // --- luz direccional ---
    vec3 lightDir = normalize(-dirLight.direction);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 result = dirLight.ambient + dirLight.diffuse * diff + dirLight.specular * spec * specStrength;

    // --- reflectores del estadio ---
    for (int i = 0; i < numSpots; i++)
    {
        vec3 L = normalize(spotLights[i].position - FragPos);
        float theta = dot(L, normalize(-spotLights[i].direction));
        float epsilon = spotLights[i].cutOff - spotLights[i].outerCutOff;
        float intensity = clamp((theta - spotLights[i].outerCutOff) / epsilon, 0.0, 1.0);
        float d = length(spotLights[i].position - FragPos);
        float att = 1.0 / (spotLights[i].constant + spotLights[i].linear * d + spotLights[i].quadratic * d * d);
        float dS = max(dot(norm, L), 0.0);
        result += spotLights[i].diffuse * dS * intensity * att;
    }

    // --- luz puntual del tunel ---
    vec3 Lp = pointPos - FragPos;
    float dp = length(Lp);
    result += pointColor * max(dot(norm, normalize(Lp)), 0.0) / (1.0 + 0.09 * dp + 0.032 * dp * dp);

    color = vec4(result * texColor.rgb, texColor.a * opacity);
}
