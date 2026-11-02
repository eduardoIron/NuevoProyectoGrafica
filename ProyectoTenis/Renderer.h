// ============================================================================
//  Renderer.h  -  Materiales, iluminación, sombras, cielo y dibujado
//
//  Pasadas por cuadro:
//    1) Sombras: profundidad de la escena desde el Sol/Luna (ortográfica, 4096^2)
//    2) Cielo procedural (gradiente día/noche, Sol, Luna, estrellas, nubes)
//    3) Escena: Blinn-Phong + luz direccional + 4 reflectores (spot) + 2 puntuales
//       + sombras PCF + normal mapping + niebla + tone mapping + gamma
//    4) Brillos aditivos de los reflectores y HUD
// ============================================================================
#pragma once

#include <GL/glew.h>
#include "Utils.h"
#include "Shader.h"
#include "Mesh.h"
#include "Model.h"
#include <vector>

struct Material
{
    glm::vec3 color = glm::vec3(1.0f);
    float specular = 0.10f;
    float shininess = 32.0f;
    glm::vec3 emissive = glm::vec3(0.0f);
    float selfIllum = 0.0f;         // >0: la textura se ve "encendida" (pantallas, letreros)
    float alpha = 1.0f;
    GLuint tex = 0;                 // difusa (0 = blanca)
    GLuint normalTex = 0;           // mapa de normales (0 = sin relieve)
    GLuint decalTex = 0;            // líneas de la cancha (0 = no aplica)
    float normalStrength = 1.0f;
    glm::vec2 uvScale = glm::vec2(1.0f);
    glm::vec3 outsideTint = glm::vec3(1.0f);
    bool alphaTest = false;
    bool blend = false;
    bool doubleSided = false;
    bool castShadow = true;

    Material() {}
    explicit Material(const glm::vec3& c, float spec = 0.10f, float shin = 32.0f) : color(c), specular(spec), shininess(shin) {}
};

struct SpotLight { glm::vec3 pos, dir, color; float innerCos, outerCos; };
struct PointLight { glm::vec3 pos, color; };

struct Lighting
{
    glm::vec3 lightDir = glm::vec3(0, 1, 0);       // HACIA la luz principal (Sol o Luna)
    glm::vec3 lightColor = glm::vec3(1.0f);
    glm::vec3 sunDir = glm::vec3(0, 1, 0);         // posición real del Sol (puede estar bajo el horizonte)
    glm::vec3 moonDir = glm::vec3(0, -1, 0);
    float dayFactor = 1.0f;                        // 0 = noche, 1 = día
    glm::vec3 ambSky = glm::vec3(0.4f), ambGround = glm::vec3(0.2f);
    glm::vec3 fogColor = glm::vec3(0.6f);
    float fogDensity = 0.002f;
    float exposure = 1.0f;
    glm::mat4 lightSpace = glm::mat4(1.0f);
    float floodLevel = 0.0f;                       // 0..1 intensidad de los reflectores
    int nSpots = 0;
    SpotLight spots[4];
    int nPoints = 0;
    PointLight points[3];
};

// Calcula el estado de luces para una hora del día (0-24) y un nivel de reflectores (0..1).
Lighting computeLighting(float hour, float floodLevel, const std::vector<glm::vec3>& towerLamps,
                         const std::vector<PointLight>& points);

class Renderer
{
public:
    bool init();

    // --- pasada de sombras ---
    void beginShadow(const Lighting& L);
    void endShadow(int viewportW, int viewportH);

    // --- pasada principal ---
    void beginScene(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& eye, const Lighting& L,
                    float time, int viewportW, int viewportH);
    void drawSky(const Lighting& L, float time);

    void draw(const Mesh& m, const glm::mat4& model, const Material& mat);
    void drawModel(const Model& mdl, const glm::mat4& model, const glm::vec3& tint = glm::vec3(1.0f));
    // Público (malla instanciada)
    void drawCrowd(const Mesh& m, int count, float time, float cheer, float waveOn, float waveTime);
    // Brillos aditivos tipo "halo" alrededor de las lámparas
    void drawGlow(const std::vector<glm::vec3>& centers, float size, const glm::vec3& color);

    bool shadowPass() const { return shadowPass_; }
    GLuint shadowTexture() const { return shadowTex_; }

private:
    Shader scene_, crowd_, depth_, sky_, glow_;
    GLuint shadowFbo_ = 0, shadowTex_ = 0, dummyVao_ = 0;
    int shadowSize_ = 4096;
    bool shadowPass_ = false;
    glm::mat4 view_, proj_;
    glm::vec3 eye_;
    Lighting L_;

    void uploadLighting(const Shader& s);
    void setMaterial(const Shader& s, const Material& m);
};
