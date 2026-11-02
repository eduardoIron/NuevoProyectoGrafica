// ============================================================================
//  Personaje.h  -  Personajes articulados (MODELADO JERÁRQUICO)
//
//  Cada personaje es un árbol de transformaciones, igual que el brazo de la
//  práctica "Modelado Jerárquico" pero con cuerpo completo:
//
//      raíz (posición + giro)
//       └─ cadera ── torso ── cuello ── cabeza
//                      ├─ hombro D ── codo D ── muñeca D ── mano D ── RAQUETA (modelo .obj)
//                      └─ hombro I ── codo I ── muñeca I ── mano I
//           ├─ cadera D ── rodilla D ── tobillo D ── pie D
//           └─ cadera I ── rodilla I ── tobillo I ── pie I
//
//  El estado de cada articulación es un ángulo (grados) dentro de una "Pose".
//  Las animaciones por keyframes interpolan Poses; los pies se apoyan en el piso
//  automáticamente a partir de los ángulos de pierna.
// ============================================================================
#pragma once

#include "Utils.h"
#include "Mesh.h"
#include "Renderer.h"
#include "Model.h"

enum Joint
{
    J_LEAN = 0,                                   // inclinación de la cadera/cuerpo (adelante +)
    J_JUMP,                                       // elevación extra (metros)
    J_HIPY,                                       // altura manual de la cadera (solo si no hay auto-apoyo)
    J_TORSO_PITCH, J_TORSO_YAW, J_TORSO_ROLL,
    J_HEAD_YAW, J_HEAD_PITCH,
    J_RSH_PITCH, J_RSH_YAW, J_RSH_ROLL, J_RELB, J_RWR_PITCH, J_RWR_YAW,
    J_LSH_PITCH, J_LSH_YAW, J_LSH_ROLL, J_LELB, J_LWR_PITCH,
    J_RHIP, J_RHIP_ROLL, J_RKNEE, J_RANKLE,
    J_LHIP, J_LHIP_ROLL, J_LKNEE, J_LANKLE,
    J_COUNT
};

struct Pose
{
    float a[J_COUNT];
    Pose() { for (int i = 0; i < J_COUNT; ++i) a[i] = 0.0f; }
    float& operator[](int j) { return a[j]; }
    float operator[](int j) const { return a[j]; }
    static Pose mix(const Pose& p, const Pose& q, float t)
    {
        Pose r;
        for (int i = 0; i < J_COUNT; ++i) r.a[i] = p.a[i] + (q.a[i] - p.a[i]) * t;
        return r;
    }
};

struct Outfit
{
    glm::vec3 skin = glm::vec3(0.88f, 0.68f, 0.54f);
    glm::vec3 hair = glm::vec3(0.12f, 0.08f, 0.05f);
    glm::vec3 shirt = glm::vec3(0.95f), trim = glm::vec3(0.1f);
    glm::vec3 shorts = glm::vec3(0.1f, 0.15f, 0.4f);
    glm::vec3 shoes = glm::vec3(0.95f), socks = glm::vec3(0.95f);
    glm::vec3 cap = glm::vec3(0.95f);
    bool hasCap = false, hasBand = false, longPants = false;
    float scale = 1.0f;
};

struct Skeleton
{
    glm::mat4 root, hips, torso, neck, head;
    glm::mat4 rSh, rElb, rWr, lSh, lElb, lWr;
    glm::mat4 rHip, rKnee, rAnkle, lHip, lKnee, lAnkle;
    glm::mat4 racket;                      // origen = empuñadura, +Y hacia la cabeza
};

class Character
{
public:
    Outfit outfit;
    glm::vec3 pos = glm::vec3(0.0f);
    float yaw = 0.0f;                      // grados; 0 = mira a +Z
    Pose pose;
    bool autoGround = true;                // apoyar pies en el piso según la pose
    bool hasRacket = true;
    bool hideHead = false;                 // para la cámara en primera persona

    Skeleton skeleton() const;
    void draw(Renderer& r, const Model* racket) const;

    // consultas en coordenadas de mundo
    glm::vec3 racketCenter(const Skeleton& s) const;
    glm::vec3 handPos(const Skeleton& s, bool right) const;
    glm::vec3 headPos(const Skeleton& s) const;
    glm::vec3 forward() const;
};
