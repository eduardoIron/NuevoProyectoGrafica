#include "Personaje.h"
#include <algorithm>

typedef glm::vec3 V3;
typedef glm::mat4 M4;

namespace
{
    const float THIGH = 0.44f, SHIN = 0.43f, ANKLE_H = 0.08f;
    const float UPPER_ARM = 0.29f, FOREARM = 0.26f;

    M4 T(float x, float y, float z) { return glm::translate(M4(1.0f), V3(x, y, z)); }
    M4 RX(float d) { return glm::rotate(M4(1.0f), U::rad(d), V3(1, 0, 0)); }
    M4 RY(float d) { return glm::rotate(M4(1.0f), U::rad(d), V3(0, 1, 0)); }
    M4 RZ(float d) { return glm::rotate(M4(1.0f), U::rad(d), V3(0, 0, 1)); }
    M4 S(float x, float y, float z) { return glm::scale(M4(1.0f), V3(x, y, z)); }

    // Mallas unitarias compartidas
    struct Parts
    {
        Mesh cube, cylDown, limb, sph;
        bool ready;
        Parts() : ready(false) {}
        void init()
        {
            if (ready) return;
            { MeshBuilder b; b.box(M4(1.0f), V3(1.0f)); cube = b.build(); }
            { MeshBuilder b; b.cylinder(T(0, -1, 0), 1.0f, 1.0f, 1.0f, 14, true); cylDown = b.build(); }
            { MeshBuilder b; b.cylinder(T(0, -1, 0), 0.70f, 1.0f, 1.0f, 14, true); limb = b.build(); }
            { MeshBuilder b; b.sphere(M4(1.0f), 1.0f, 16, 12); sph = b.build(); }
            ready = true;
        }
    };
    Parts g_parts;

    void drawPart(Renderer& r, const Mesh& m, const M4& M, const V3& sc, const V3& col, float spec = 0.12f)
    {
        Material mat(col, spec, 24.0f);
        r.draw(m, M * S(sc.x, sc.y, sc.z), mat);
    }
    void box(Renderer& r, const M4& M, const V3& size, const V3& col, float spec = 0.12f) { drawPart(r, g_parts.cube, M, size, col, spec); }
    void ball(Renderer& r, const M4& M, const V3& size, const V3& col, float spec = 0.12f) { drawPart(r, g_parts.sph, M, size, col, spec); }
    // cilindro que cuelga desde el origen hacia -Y, de longitud len
    void cyl(Renderer& r, const M4& M, float rad, float len, const V3& col, float spec = 0.12f) { drawPart(r, g_parts.cylDown, M, V3(rad, len, rad), col, spec); }
    // miembro ahusado (más ancho arriba)
    void limb(Renderer& r, const M4& M, float rad, float len, const V3& col, float spec = 0.12f) { drawPart(r, g_parts.limb, M, V3(rad, len, rad), col, spec); }

    // altura del suelo para que los pies toquen y=0
    float groundOffset(const Pose& p)
    {
        float best = 1e9f;
        for (int side = 0; side < 2; ++side)
        {
            float hip = side == 0 ? p[J_RHIP] : p[J_LHIP];
            float roll = side == 0 ? p[J_RHIP_ROLL] : p[J_LHIP_ROLL];
            float knee = side == 0 ? p[J_RKNEE] : p[J_LKNEE];
            float ank = side == 0 ? p[J_RANKLE] : p[J_LANKLE];
            float sx = side == 0 ? -1.0f : 1.0f;
            M4 m = RX(p[J_LEAN]) * T(sx * 0.095f, -0.04f, 0) * RZ(sx * -roll) * RX(-hip);
            m = m * T(0, -THIGH, 0) * RX(knee);
            m = m * T(0, -SHIN, 0) * RX(-ank);
            // punto más bajo del pie (talón y punta)
            float heel = (m * glm::vec4(0, -ANKLE_H, -0.06f, 1.0f)).y;
            float toe = (m * glm::vec4(0, -ANKLE_H, 0.17f, 1.0f)).y;
            float lowest = std::min(heel, toe);
            best = std::min(best, lowest);
        }
        return -best;
    }
}

Skeleton Character::skeleton() const
{
    g_parts.init();
    Skeleton s;
    const float sc = outfit.scale;
    s.root = T(pos.x, pos.y, pos.z) * RY(yaw) * S(sc, sc, sc);
    float hy = autoGround ? groundOffset(pose) : pose[J_HIPY];
    s.hips = s.root * T(0, hy + pose[J_JUMP], 0) * RX(pose[J_LEAN]);
    s.torso = s.hips * T(0, 0.08f, 0) * RY(pose[J_TORSO_YAW]) * RX(pose[J_TORSO_PITCH]) * RZ(pose[J_TORSO_ROLL]);
    s.neck = s.torso * T(0, 0.56f, 0);
    s.head = s.neck * T(0, 0.05f, 0) * RY(pose[J_HEAD_YAW]) * RX(pose[J_HEAD_PITCH]);

    // brazo derecho (el lado derecho del personaje está en -X)
    s.rSh = s.torso * T(-0.205f, 0.50f, 0) * RY(pose[J_RSH_YAW]) * RX(-pose[J_RSH_PITCH]) * RZ(-pose[J_RSH_ROLL]);
    s.rElb = s.rSh * T(0, -UPPER_ARM, 0) * RX(-pose[J_RELB]);
    s.rWr = s.rElb * T(0, -FOREARM, 0) * RY(pose[J_RWR_YAW]) * RX(-pose[J_RWR_PITCH]);
    s.lSh = s.torso * T(0.205f, 0.50f, 0) * RY(pose[J_LSH_YAW]) * RX(-pose[J_LSH_PITCH]) * RZ(pose[J_LSH_ROLL]);
    s.lElb = s.lSh * T(0, -UPPER_ARM, 0) * RX(-pose[J_LELB]);
    s.lWr = s.lElb * T(0, -FOREARM, 0) * RX(-pose[J_LWR_PITCH]);

    s.rHip = s.hips * T(-0.095f, -0.04f, 0) * RZ(-pose[J_RHIP_ROLL]) * RX(-pose[J_RHIP]);
    s.rKnee = s.rHip * T(0, -THIGH, 0) * RX(pose[J_RKNEE]);
    s.rAnkle = s.rKnee * T(0, -SHIN, 0) * RX(-pose[J_RANKLE]);
    s.lHip = s.hips * T(0.095f, -0.04f, 0) * RZ(pose[J_LHIP_ROLL]) * RX(-pose[J_LHIP]);
    s.lKnee = s.lHip * T(0, -THIGH, 0) * RX(pose[J_LKNEE]);
    s.lAnkle = s.lKnee * T(0, -SHIN, 0) * RX(-pose[J_LANKLE]);

    // raqueta: la empuñadura está en la mano y la cabeza continúa la dirección del antebrazo
    s.racket = s.rWr * T(0, -0.055f, 0) * RX(180.0f);
    return s;
}

glm::vec3 Character::racketCenter(const Skeleton& s) const { return V3(s.racket * glm::vec4(0, 0.49f, 0, 1)); }
glm::vec3 Character::handPos(const Skeleton& s, bool right) const { return V3((right ? s.rWr : s.lWr) * glm::vec4(0, -0.05f, 0.0f, 1)); }
glm::vec3 Character::headPos(const Skeleton& s) const { return V3(s.head * glm::vec4(0, 0.11f, 0, 1)); }
glm::vec3 Character::forward() const { return V3(std::sin(U::rad(yaw)), 0.0f, std::cos(U::rad(yaw))); }

void Character::draw(Renderer& r, const Model* racket) const
{
    Skeleton s = skeleton();
    const Outfit& o = outfit;

    // ---- pelvis y pantalón corto ----
    box(r, s.hips, V3(0.32f, 0.17f, 0.21f), o.shorts);
    // ---- torso ----
    box(r, s.torso * T(0, 0.27f, 0), V3(0.35f, 0.55f, 0.20f), o.shirt, 0.08f);
    box(r, s.torso * T(0, 0.43f, 0), V3(0.40f, 0.24f, 0.22f), o.shirt, 0.08f);          // pecho/hombros
    box(r, s.torso * T(0, 0.05f, 0), V3(0.355f, 0.04f, 0.205f), o.trim, 0.08f);          // franja
    ball(r, s.torso * T(-0.205f, 0.50f, 0), V3(0.07f), o.shirt, 0.08f);
    ball(r, s.torso * T(0.205f, 0.50f, 0), V3(0.07f), o.shirt, 0.08f);
    // ---- cuello y cabeza ----
    cyl(r, s.neck * T(0, 0.05f, 0), 0.04f, 0.09f, o.skin);
    if (!hideHead)
    {
        M4 H = s.head * T(0, 0.115f, 0.005f);
        ball(r, H, V3(0.100f, 0.118f, 0.108f), o.skin);
        ball(r, H * T(-0.037f, 0.012f, 0.098f), V3(0.012f), V3(0.05f, 0.05f, 0.06f), 0.4f);   // ojos
        ball(r, H * T(0.037f, 0.012f, 0.098f), V3(0.012f), V3(0.05f, 0.05f, 0.06f), 0.4f);
        ball(r, H * T(0, -0.012f, 0.108f), V3(0.016f, 0.020f, 0.016f), o.skin * 0.95f);       // nariz
        if (o.hasCap)
        {
            ball(r, H * T(0, 0.035f, -0.006f), V3(0.109f, 0.082f, 0.116f), o.cap, 0.10f);
            box(r, H * T(0, 0.030f, 0.108f) * RX(-8.0f), V3(0.15f, 0.014f, 0.085f), o.cap, 0.10f);   // visera
        }
        else
        {
            ball(r, H * T(0, 0.040f, -0.012f), V3(0.106f, 0.088f, 0.112f), o.hair, 0.08f);
            if (o.hasBand) box(r, H * T(0, 0.040f, 0.0f), V3(0.222f, 0.026f, 0.224f), o.trim, 0.08f);
        }
    }
    // ---- brazos ----
    for (int side = 0; side < 2; ++side)
    {
        const M4& Sh = side == 0 ? s.rSh : s.lSh;
        const M4& El = side == 0 ? s.rElb : s.lElb;
        const M4& Wr = side == 0 ? s.rWr : s.lWr;
        limb(r, Sh, 0.043f, UPPER_ARM, o.skin);
        cyl(r, Sh * T(0, -0.01f, 0), 0.052f, 0.15f, o.shirt, 0.08f);                  // manga corta
        ball(r, El, V3(0.038f), o.skin);
        limb(r, El, 0.037f, FOREARM, o.skin);
        cyl(r, Wr * T(0, 0.06f, 0), 0.040f, 0.05f, o.trim, 0.1f);                     // muñequera
        ball(r, Wr * T(0, -0.045f, 0), V3(0.044f, 0.052f, 0.040f), o.skin);           // mano
    }
    // ---- piernas ----
    for (int side = 0; side < 2; ++side)
    {
        const M4& Hp = side == 0 ? s.rHip : s.lHip;
        const M4& Kn = side == 0 ? s.rKnee : s.lKnee;
        const M4& An = side == 0 ? s.rAnkle : s.lAnkle;
        limb(r, Hp, 0.075f, THIGH, o.skin);
        cyl(r, Hp * T(0, 0.02f, 0), 0.084f, o.longPants ? 0.46f : 0.26f, o.shorts);   // pierna del short / pantalón
        ball(r, Kn, V3(0.055f), o.skin);
        limb(r, Kn, 0.055f, SHIN, o.skin);
        if (o.longPants) cyl(r, Kn, 0.060f, SHIN, o.shorts);
        else cyl(r, Kn * T(0, -SHIN + 0.20f, 0), 0.052f, 0.20f, o.socks, 0.05f);     // calcetín
        box(r, An * T(0, -0.045f, 0.045f), V3(0.105f, 0.075f, 0.265f), o.shoes, 0.2f); // tenis
        box(r, An * T(0, -0.085f, 0.045f), V3(0.108f, 0.016f, 0.268f), V3(0.12f), 0.05f); // suela
    }
    // ---- raqueta (modelo cargado con Assimp) ----
    if (hasRacket && racket) r.drawModel(*racket, s.racket);
}
