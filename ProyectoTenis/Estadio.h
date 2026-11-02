// ============================================================================
//  Estadio.h  -  Escenario: cancha, red, gradas con público, túnel de vestidores,
//                silla del juez, zona de descanso, torres de iluminación, marcadores
//
//  Zonas funcionales (requisito del proyecto):
//    1. Terreno de juego      (cancha + muro de publicidad + red)
//    2. Zona de descanso      (sillas, sombrillas, hieleras, toallas, botellas)
//    3. Silla del juez        (estructura elevada, escalera, micrófono, panel)
//    4. Vestidores / túnel    (pasillo con señalización e iluminación interior)
//    5. Gradas con público    (4 tribunas, ~2,600 espectadores instanciados)
// ============================================================================
#pragma once

#include "Utils.h"
#include "Mesh.h"
#include "Renderer.h"
#include "Texturas.h"
#include "Model.h"
#include <vector>

enum Surface { SURF_CLAY = 0, SURF_GRASS = 1, SURF_HARD = 2, SURF_COUNT = 3 };

// Puntos de interés para cámaras, personajes y luces
struct Anchors
{
    glm::vec3 umpireSeat;              // centro de la cadera del juez sentado
    glm::vec3 umpireEye;
    glm::vec3 tunnelMouth;
    glm::vec3 restChair[2];            // sillas de los jugadores
    glm::vec3 kidCorner[2];            // esquinas de los recogepelotas
    glm::vec3 tvCam[2];                // posiciones de la cámara de transmisión
    glm::vec3 netCenter;
    glm::vec3 scoreboardCenter;
    std::vector<glm::vec3> towerLamps; // origen de los 4 reflectores (spots)
    std::vector<glm::vec3> glowCenters;
    std::vector<PointLight> points;    // luces puntuales (túnel)
};

// Estado dinámico que el estadio necesita al dibujar
struct StadiumDraw
{
    int surface = SURF_CLAY;
    float time = 0.0f;
    float scorePulse = 0.0f;           // 0..1 destello del marcador al cambiar
    float cheer = 0.0f;                // 0..1 festejo del público
    float waveOn = 0.0f;
    float waveTime = 0.0f;
    float floodLevel = 0.0f;           // reflectores
};

class Estadio
{
public:
    Anchors anchors;

    void build(const Textures& tex);
    // Dibuja la parte opaca (y en la pasada de sombras solo lo que proyecta sombra)
    void drawOpaque(Renderer& r, const StadiumDraw& d, const Assets& assets);
    // Dibuja lo transparente (red) y los halos de luz
    void drawTransparent(Renderer& r, const StadiumDraw& d);

    // Altura del piso/gradas en (x,z): para que la cámara "espectador" suba por las gradas
    float groundHeight(float x, float z) const;

    int spectatorCount() const { return crowdCount_; }

private:
    struct Batch { Mesh mesh; Material mat; };
    std::vector<Batch> batches_;
    Mesh courtMesh_, netMesh_, netBandMesh_, scoreMesh_, miniScoreMesh_, crowdMesh_;
    Material surfaceMat_[SURF_COUNT];
    Material netMat_, bandMat_, scoreMat_;
    std::vector<glm::vec3> ballSpots_;   // pelotas esparcidas en la cancha
    glm::mat4 chairXf_[2];               // marco de las sillas de descanso
    int crowdCount_ = 0;
    const Textures* tex_ = 0;

    void buildStand(MeshBuilder& conc, MeshBuilder& seats, std::vector<float>& crowd,
                    const glm::vec3& origin, float yawDeg, float halfLen, int rows, bool tunnelCut,
                    int palette, uint32_t seed);
    void buildCourtAndWalls(MeshBuilder& conc, MeshBuilder& ads, MeshBuilder& props);
    void buildTunnel(MeshBuilder& conc, MeshBuilder& lamps, MeshBuilder& signs);
    void buildNet(MeshBuilder& props);
    void buildUmpireChair(MeshBuilder& props, MeshBuilder& screens);
    void buildRestZone(MeshBuilder& props, MeshBuilder& canopy);
    void buildTowers(MeshBuilder& props, MeshBuilder& lamps);
    void buildScoreboard(MeshBuilder& props);
    void buildCameras(MeshBuilder& props);
    void buildFlags(MeshBuilder& props);
    void buildSpectatorMesh();

    void addBatch(const MeshBuilder& mb, const Material& m);
};
