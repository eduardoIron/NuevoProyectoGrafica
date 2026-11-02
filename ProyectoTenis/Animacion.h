// ============================================================================
//  Animacion.h  -  Animación por KEYFRAMES de poses articuladas
//
//  Una animación es una lista de "PoseKey" (tiempo + pose completa). Entre dos
//  keyframes se interpola con una spline de Hermite (tangentes por diferencias
//  finitas), de modo que el movimiento es continuo y pasa por cada pose clave.
//  Hay animaciones de: saque, derecha (forehand), estirada, festejo, desánimo,
//  recoger pelota, y un ciclo de caminata/carrera procedural.
// ============================================================================
#pragma once

#include "Personaje.h"
#include <vector>

struct PoseKey
{
    float t;
    Pose p;
};

namespace Anim
{
    Pose evalTrack(const std::vector<PoseKey>& keys, float t);
    void flatFeet(Pose& p);                                    // tobillos para que el pie quede plano
    // Superpone el ciclo de marcha sobre una pose: phase (rad), speed (m/s)
    void locomotion(Pose& p, float phase, float speed, bool keepRightArm);

    Pose rest();
    Pose ready();
    Pose umpireSeated();
    Pose umpireAnnounce();
    Pose kidCrouch();
    Pose kidReady();

    const std::vector<PoseKey>& serveTrack();
    const std::vector<PoseKey>& forehandTrack();
    const std::vector<PoseKey>& lungeTrack();
    const std::vector<PoseKey>& celebrateTrack();
    const std::vector<PoseKey>& dejectTrack();
    const std::vector<PoseKey>& pickTrack();

    // tiempos característicos (segundos)
    const float SERVE_RELEASE = 0.92f, SERVE_CONTACT = 1.55f, SERVE_TOTAL = 3.0f;
    const float FH_CONTACT = 0.46f, FH_TOTAL = 1.25f;
    const float LUNGE_CONTACT = 0.45f, LUNGE_TOTAL = 1.1f;
    const float CELEBRATE_TOTAL = 2.6f, DEJECT_TOTAL = 2.2f, PICK_TOTAL = 1.0f;
}
