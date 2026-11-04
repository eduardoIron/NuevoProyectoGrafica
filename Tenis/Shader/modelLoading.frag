#version 330 core
// Sin iluminacion (marcador, lamparas): solo la textura
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform float brillo;

void main()
{
    vec4 texColor = texture(texture_diffuse1, TexCoords);
    if (texColor.a < 0.1)
        discard;
    FragColor = vec4(texColor.rgb * brillo, texColor.a);
}
