#include "Camara.h"
#include <algorithm>

namespace
{
    glm::vec3 dirFromAngles(float yawDeg, float pitchDeg)
    {
        float y = U::rad(yawDeg), p = U::rad(pitchDeg);
        return glm::normalize(glm::vec3(std::cos(y) * std::cos(p), std::sin(p), std::sin(y) * std::cos(p)));
    }
    float yawTo(const glm::vec3& d) { return U::deg(std::atan2(d.z, d.x)); }
    float pitchTo(const glm::vec3& d) { return U::deg(std::atan2(d.y, std::sqrt(d.x * d.x + d.z * d.z))); }
}

const char* CameraRig::modeName() const
{
    static const char* names[CAM_COUNT] = { "LIBRE", "ESPECTADOR", "JUEZ DE SILLA (1RA PERSONA)", "TRANSMISION TV",
                                            "TERCERA PERSONA", "CENITAL", "JUGADOR (1RA PERSONA)" };
    return names[mode];
}

void CameraRig::init(const CamScene& s)
{
    (void)s;
    setMode(CAM_LIBRE, s);
}

void CameraRig::setMode(CamMode m, const CamScene& s)
{
    mode = m;
    option_ = 0;
    lookYaw_ = lookPitch_ = 0.0f;
    switch (m)
    {
    case CAM_LIBRE:      fov = 55.0f; break;
    case CAM_ESPECTADOR: fov = 65.0f; break;
    case CAM_JUEZ:       fov = 60.0f; umpYaw_ = 0.0f; umpPitch_ = 0.0f; break;
    case CAM_TV:         tvFov_ = 24.0f; fov = tvFov_; tvTarget_ = s.ball; break;
    case CAM_TERCERA:    fov = 55.0f; orbDist_ = 5.0f; orbPitch_ = 18.0f; orbYaw_ = 25.0f; eye_ = s.player[0] + glm::vec3(0, 3, 6); break;
    case CAM_CENITAL:    fov = 50.0f; topHeight_ = 46.0f; break;
    case CAM_JUGADOR:    fov = 75.0f; break;
    default: break;
    }
    lastMode_ = m;
}

void CameraRig::cycleOption(const CamScene& s)
{
    (void)s;
    option_ = (option_ + 1) % 2;
    if (mode == CAM_TERCERA) orbYaw_ += 180.0f;     // pasa a la espalda del otro jugador
}

void CameraRig::setFree(const glm::vec3& pos, const glm::vec3& target)
{
    mode = CAM_LIBRE;
    freePos_ = pos;
    glm::vec3 d = glm::normalize(target - pos);
    freeYaw_ = yawTo(d);
    freePitch_ = pitchTo(d);
    eye_ = pos;
    target_ = target;
}

void CameraRig::update(float dt, const CamInput& in, const CamScene& s)
{
    const float sens = 0.12f;
    switch (mode)
    {
    case CAM_LIBRE:
    {
        freeYaw_ += in.mdx * sens;
        freePitch_ = U::clampf(freePitch_ - in.mdy * sens, -89.0f, 89.0f);
        glm::vec3 f = dirFromAngles(freeYaw_, freePitch_);
        glm::vec3 r = glm::normalize(glm::cross(f, glm::vec3(0, 1, 0)));
        float sp = (in.fast ? 22.0f : 8.0f) * dt;
        freePos_ += f * (in.forward * sp) + r * (in.right * sp) + glm::vec3(0, 1, 0) * (in.up * sp);
        freePos_.y = std::max(freePos_.y, 0.3f);
        freePos_.x = U::clampf(freePos_.x, -90.0f, 90.0f);
        freePos_.z = U::clampf(freePos_.z, -90.0f, 90.0f);
        freePos_.y = U::clampf(freePos_.y, 0.3f, 70.0f);
        eye_ = freePos_;
        target_ = eye_ + f;
        fov = U::clampf(fov - in.scroll * 2.0f, 20.0f, 90.0f);
        break;
    }
    case CAM_ESPECTADOR:
    {
        walkYaw_ += in.mdx * sens;
        walkPitch_ = U::clampf(walkPitch_ - in.mdy * sens, -80.0f, 80.0f);
        glm::vec3 f = dirFromAngles(walkYaw_, 0.0f);                 // movimiento horizontal
        glm::vec3 r = glm::normalize(glm::cross(f, glm::vec3(0, 1, 0)));
        float sp = (in.fast ? 9.0f : 4.0f) * dt;
        walkPos_ += f * (in.forward * sp) + r * (in.right * sp);
        walkPos_.x = U::clampf(walkPos_.x, -40.0f, 40.0f);
        walkPos_.z = U::clampf(walkPos_.z, -44.0f, 44.0f);
        float gh = s.groundHeight ? s.groundHeight(s.ctx, walkPos_.x, walkPos_.z) : 0.0f;
        float targetY = gh + 1.70f;
        walkPos_.y += (targetY - walkPos_.y) * U::damp(10.0f, dt);   // sube/baja suavemente las gradas
        eye_ = walkPos_;
        target_ = eye_ + dirFromAngles(walkYaw_, walkPitch_);
        fov = U::clampf(fov - in.scroll * 2.0f, 35.0f, 90.0f);
        break;
    }
    case CAM_JUEZ:
    {
        // la cabeza del juez sigue la pelota; el mouse añade una pequeña desviación
        lookYaw_ = U::clampf(lookYaw_ + in.mdx * sens, -35.0f, 35.0f);
        lookPitch_ = U::clampf(lookPitch_ - in.mdy * sens, -25.0f, 25.0f);
        glm::vec3 d = s.ball - s.umpireEye;
        float ty = yawTo(d), tp = pitchTo(d);
        float k = U::damp(4.0f, dt);
        umpYaw_ += U::wrap180(ty - umpYaw_) * k;
        umpPitch_ += (tp - umpPitch_) * k;
        eye_ = s.umpireEye;
        // limitar el giro a +-70 grados alrededor de mirar al centro (-X)
        float base = 180.0f;
        float rel = U::clampf(U::wrap180(umpYaw_ - base), -70.0f, 70.0f);
        target_ = eye_ + dirFromAngles(base + rel + lookYaw_, U::clampf(umpPitch_, -35.0f, 15.0f) + lookPitch_);
        fov = U::clampf(fov - in.scroll * 2.0f, 30.0f, 85.0f);
        break;
    }
    case CAM_TV:
    {
        eye_ = s.tvPos[option_ % 2];
        // el objetivo persigue la pelota y el punto medio de los jugadores
        glm::vec3 mid = (s.player[0] + s.player[1]) * 0.5f + glm::vec3(0, 1.0f, 0);
        glm::vec3 want = glm::mix(mid, s.ball, 0.55f);
        tvTarget_ += (want - tvTarget_) * U::damp(2.2f, dt);
        target_ = tvTarget_;
        tvFov_ = U::clampf(tvFov_ - in.scroll * 1.5f, 8.0f, 60.0f);
        fov = tvFov_;
        break;
    }
    case CAM_TERCERA:
    {
        orbYaw_ += in.mdx * sens * 1.4f;
        orbPitch_ = U::clampf(orbPitch_ + in.mdy * sens, -5.0f, 80.0f);
        orbDist_ = U::clampf(orbDist_ - in.scroll * 0.6f, 2.0f, 22.0f);
        glm::vec3 c = s.player[option_ % 2] + glm::vec3(0, 1.2f, 0);
        glm::vec3 off(std::cos(U::rad(orbPitch_)) * std::sin(U::rad(orbYaw_)),
                      std::sin(U::rad(orbPitch_)),
                      std::cos(U::rad(orbPitch_)) * std::cos(U::rad(orbYaw_)));
        glm::vec3 want = c + off * orbDist_;
        // la cámara no atraviesa el muro de publicidad: se limita al interior de la cancha
        want.x = U::clampf(want.x, -10.0f, 10.0f);
        want.z = U::clampf(want.z, -18.4f, 18.4f);
        want.y = U::clampf(want.y, 0.5f, 12.0f);
        eye_ += (want - eye_) * U::damp(9.0f, dt);
        target_ = c;
        break;
    }
    case CAM_CENITAL:
    {
        topYaw_ += in.mdx * sens;
        topHeight_ = U::clampf(topHeight_ - in.scroll * 3.0f, 16.0f, 90.0f);
        eye_ = glm::vec3(0.0f, topHeight_, 0.0f);
        glm::vec3 look = glm::vec3(std::sin(U::rad(topYaw_)), 0, -std::cos(U::rad(topYaw_)));
        target_ = glm::vec3(0.0f, 0.0f, 0.0f) + look * 0.001f;
        break;
    }
    case CAM_JUGADOR:
    {
        lookYaw_ = U::clampf(lookYaw_ + in.mdx * sens, -50.0f, 50.0f);
        lookPitch_ = U::clampf(lookPitch_ - in.mdy * sens, -40.0f, 40.0f);
        glm::vec3 fwd = glm::normalize(glm::vec3(s.playerFwd.x, 0.0f, s.playerFwd.z));
        float baseYaw = yawTo(fwd);
        glm::vec3 d = s.ball - s.playerHead;
        float ty = yawTo(d), tp = pitchTo(d);
        float rel = U::clampf(U::wrap180(ty - baseYaw), -75.0f, 75.0f);
        float k = U::damp(5.0f, dt);
        umpYaw_ += (rel - umpYaw_) * k;
        umpPitch_ += (U::clampf(tp, -45.0f, 25.0f) - umpPitch_) * k;
        eye_ = s.playerHead + fwd * 0.10f;
        target_ = eye_ + dirFromAngles(baseYaw + umpYaw_ + lookYaw_, umpPitch_ + lookPitch_);
        break;
    }
    default: break;
    }
}

glm::mat4 CameraRig::view() const
{
    glm::vec3 up(0, 1, 0);
    if (mode == CAM_CENITAL)
    {
        up = glm::vec3(std::sin(U::rad(topYaw_)), 0, -std::cos(U::rad(topYaw_)));
        return glm::lookAt(eye_, glm::vec3(0.0f), up);
    }
    return glm::lookAt(eye_, target_, up);
}
