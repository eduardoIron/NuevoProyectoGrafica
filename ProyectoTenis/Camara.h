// ============================================================================
//  Camara.h  -  Sistema de cámaras (7 modos)
//
//   1 Libre            vuelo libre con WASD + mouse (estilo FreeCam de las prácticas)
//   2 Espectador       primera persona caminando a 1.7 m; sube por las gradas
//   3 Juez de silla    primera persona FIJA desde la silla; la cabeza sigue la pelota
//   4 Transmisión TV   cámara estática en las gradas con paneo/zoom automático
//   5 Tercera persona  órbita alrededor de un jugador (mouse = girar, rueda = distancia)
//   6 Cenital          vista aérea de la cancha
//   7 Jugador          primera persona desde la cabeza del jugador 1
// ============================================================================
#pragma once

#include "Utils.h"

enum CamMode { CAM_LIBRE = 0, CAM_ESPECTADOR, CAM_JUEZ, CAM_TV, CAM_TERCERA, CAM_CENITAL, CAM_JUGADOR, CAM_COUNT };

struct CamInput
{
    float forward = 0, right = 0, up = 0;     // -1..1
    bool fast = false;
    float mdx = 0, mdy = 0;                   // movimiento del mouse (px) en este cuadro
    float scroll = 0;                         // rueda
};

// Información de la escena que necesitan las cámaras
struct CamScene
{
    glm::vec3 ball = glm::vec3(0.0f);
    glm::vec3 player[2];
    glm::vec3 playerHead = glm::vec3(0.0f);   // cabeza del jugador 1
    glm::vec3 playerFwd = glm::vec3(0, 0, -1);
    glm::vec3 umpireEye = glm::vec3(0.0f);
    glm::vec3 tvPos[2];
    float (*groundHeight)(void* ctx, float x, float z) = 0;
    void* ctx = 0;
};

class CameraRig
{
public:
    CamMode mode = CAM_LIBRE;
    float fov = 55.0f;                        // grados (se ajusta por modo)

    void init(const CamScene& s);
    void setMode(CamMode m, const CamScene& s);
    void cycleOption(const CamScene& s);      // tecla C: posición de TV / jugador a seguir
    void setFree(const glm::vec3& pos, const glm::vec3& target);   // coloca la cámara libre (pruebas/capturas)
    void update(float dt, const CamInput& in, const CamScene& s);

    glm::mat4 view() const;
    glm::vec3 eye() const { return eye_; }
    const char* modeName() const;
    int option() const { return option_; }

private:
    glm::vec3 eye_ = glm::vec3(0, 3, 30), target_ = glm::vec3(0.0f);
    // libre / espectador / juez / jugador
    float yaw_ = -90.0f, pitch_ = -10.0f;
    float lookYaw_ = 0.0f, lookPitch_ = 0.0f;        // desviación del mouse (juez / jugador)
    glm::vec3 freePos_ = glm::vec3(0, 6, 34);
    float freeYaw_ = -90.0f, freePitch_ = -14.0f;
    glm::vec3 walkPos_ = glm::vec3(-9.0f, 1.7f, 12.0f);
    float walkYaw_ = -20.0f, walkPitch_ = 0.0f;
    // orbital
    float orbYaw_ = 200.0f, orbPitch_ = 22.0f, orbDist_ = 6.0f;
    // TV
    glm::vec3 tvTarget_ = glm::vec3(0.0f);
    float tvFov_ = 28.0f;
    // cenital
    float topYaw_ = 0.0f, topHeight_ = 46.0f;
    // juez
    float umpYaw_ = 0.0f, umpPitch_ = 0.0f;
    int option_ = 0;
    CamMode lastMode_ = CAM_LIBRE;
};
