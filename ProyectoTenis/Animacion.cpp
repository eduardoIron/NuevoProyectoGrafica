#include "Animacion.h"
#include <algorithm>
#include <initializer_list>
#include <utility>

namespace
{
    typedef std::pair<int, float> JV;

    Pose K(const Pose& base, std::initializer_list<JV> mods)
    {
        Pose p = base;
        for (std::initializer_list<JV>::const_iterator it = mods.begin(); it != mods.end(); ++it) p[it->first] = it->second;
        return p;
    }
    PoseKey key(float t, const Pose& p) { PoseKey k; k.t = t; k.p = p; return k; }
}

// ---------------------------------------------------------------------------
Pose Anim::evalTrack(const std::vector<PoseKey>& keys, float t)
{
    size_t n = keys.size();
    if (n == 0) return Pose();
    if (t <= keys[0].t) return keys[0].p;
    if (t >= keys[n - 1].t) return keys[n - 1].p;
    size_t i = 0;
    while (i + 2 < n && t >= keys[i + 1].t) ++i;
    float t0 = keys[i].t, t1 = keys[i + 1].t, h = t1 - t0;
    float u = (t - t0) / h;
    float u2 = u * u, u3 = u2 * u;
    float h00 = 2 * u3 - 3 * u2 + 1, h10 = u3 - 2 * u2 + u, h01 = -2 * u3 + 3 * u2, h11 = u3 - u2;
    Pose r;
    for (int j = 0; j < J_COUNT; ++j)
    {
        float p0 = keys[i].p[j], p1 = keys[i + 1].p[j];
        float m0 = 0.0f, m1 = 0.0f;
        if (i > 0) m0 = (p1 - keys[i - 1].p[j]) / (keys[i + 1].t - keys[i - 1].t);
        if (i + 2 < n) m1 = (keys[i + 2].p[j] - p0) / (keys[i + 2].t - keys[i].t);
        r[j] = h00 * p0 + h10 * h * m0 + h01 * p1 + h11 * h * m1;
    }
    if (r[J_JUMP] < 0.0f) r[J_JUMP] = 0.0f;
    return r;
}

void Anim::flatFeet(Pose& p)
{
    p[J_RANKLE] = U::clampf(p[J_RKNEE] - p[J_RHIP], -25.0f, 40.0f);
    p[J_LANKLE] = U::clampf(p[J_LKNEE] - p[J_LHIP], -25.0f, 40.0f);
}

void Anim::locomotion(Pose& p, float phase, float speed, bool keepRightArm)
{
    float w = U::smooth(speed / 1.2f);                         // 0 = parado, 1 = en movimiento
    float amp = U::clamp01((speed - 0.5f) / 4.0f);             // 0 = caminar, 1 = correr
    float A = U::lerp(20.0f, 44.0f, amp), kneePeak = U::lerp(38.0f, 88.0f, amp);
    float armA = U::lerp(16.0f, 42.0f, amp), elb = U::lerp(18.0f, 85.0f, amp);
    for (int side = 0; side < 2; ++side)
    {
        float ph = phase + (side == 0 ? 0.0f : U::PI);
        float hip = A * std::sin(ph) + 6.0f;
        float c = std::max(0.0f, std::cos(ph));
        float knee = 14.0f + kneePeak * std::pow(c, 1.4f);
        int HIP = side == 0 ? J_RHIP : J_LHIP, KN = side == 0 ? J_RKNEE : J_LKNEE;
        p[HIP] = U::lerp(p[HIP], hip, w);
        p[KN] = U::lerp(p[KN], knee, w);
    }
    // brazo izquierdo (y derecho si no sostiene raqueta) oscilan en contrafase
    p[J_LSH_PITCH] = U::lerp(p[J_LSH_PITCH], armA * std::sin(phase) + 5.0f, w);
    p[J_LELB] = U::lerp(p[J_LELB], elb, w);
    if (!keepRightArm)
    {
        p[J_RSH_PITCH] = U::lerp(p[J_RSH_PITCH], -armA * std::sin(phase) + 5.0f, w);
        p[J_RELB] = U::lerp(p[J_RELB], elb, w);
    }
    else
    {
        p[J_RSH_PITCH] = U::lerp(p[J_RSH_PITCH], 28.0f + 6.0f * std::sin(phase), w * 0.7f);
    }
    p[J_LEAN] = U::lerp(p[J_LEAN], U::lerp(2.0f, 15.0f, amp), w);
    p[J_TORSO_PITCH] = U::lerp(p[J_TORSO_PITCH], U::lerp(3.0f, 8.0f, amp), w);
    p[J_TORSO_YAW] = U::lerp(p[J_TORSO_YAW], 6.0f * std::sin(phase + U::PI) * (0.5f + amp), w);
}

// ---------------------------------------------------------------------------
//  Poses base
// ---------------------------------------------------------------------------
Pose Anim::rest()
{
    return K(Pose(), { {J_RELB, 12.0f}, {J_LELB, 12.0f}, {J_RSH_ROLL, 4.0f}, {J_LSH_ROLL, 4.0f}, {J_RHIP_ROLL, 2.0f}, {J_LHIP_ROLL, 2.0f} });
}

Pose Anim::ready()
{
    return K(Pose(), { {J_LEAN, 6.0f}, {J_TORSO_PITCH, 10.0f},
                       {J_RHIP, 24.0f}, {J_RKNEE, 46.0f}, {J_LHIP, 24.0f}, {J_LKNEE, 46.0f},
                       {J_RHIP_ROLL, 8.0f}, {J_LHIP_ROLL, 8.0f},
                       {J_RSH_PITCH, 38.0f}, {J_RELB, 85.0f}, {J_RWR_PITCH, 25.0f},
                       {J_LSH_PITCH, 34.0f}, {J_LELB, 80.0f}, {J_LSH_ROLL, 6.0f}, {J_RSH_ROLL, 6.0f} });
}

Pose Anim::umpireSeated()
{
    return K(Pose(), { {J_HIPY, 0.085f}, {J_TORSO_PITCH, -4.0f},
                       {J_RHIP, 58.0f}, {J_RKNEE, 55.0f}, {J_LHIP, 58.0f}, {J_LKNEE, 55.0f},
                       {J_RHIP_ROLL, 5.0f}, {J_LHIP_ROLL, 5.0f},
                       {J_RSH_PITCH, 30.0f}, {J_RELB, 80.0f}, {J_LSH_PITCH, 30.0f}, {J_LELB, 80.0f},
                       {J_RSH_ROLL, 8.0f}, {J_LSH_ROLL, 8.0f} });
}

Pose Anim::umpireAnnounce()
{
    Pose p = umpireSeated();
    p[J_RSH_PITCH] = 62.0f; p[J_RELB] = 125.0f; p[J_RWR_PITCH] = 10.0f; p[J_RSH_ROLL] = 14.0f;
    return p;
}

Pose Anim::kidCrouch()
{
    return K(Pose(), { {J_LEAN, 22.0f}, {J_TORSO_PITCH, 14.0f},
                       {J_RHIP, 92.0f}, {J_RKNEE, 135.0f}, {J_LHIP, 92.0f}, {J_LKNEE, 135.0f},
                       {J_RHIP_ROLL, 9.0f}, {J_LHIP_ROLL, 9.0f},
                       {J_RSH_PITCH, 55.0f}, {J_RELB, 25.0f}, {J_LSH_PITCH, 55.0f}, {J_LELB, 25.0f} });
}

Pose Anim::kidReady()
{
    return K(Pose(), { {J_LEAN, 4.0f}, {J_TORSO_PITCH, 6.0f},
                       {J_RHIP, 10.0f}, {J_RKNEE, 18.0f}, {J_LHIP, 10.0f}, {J_LKNEE, 18.0f},
                       {J_RHIP_ROLL, 4.0f}, {J_LHIP_ROLL, 4.0f},
                       {J_RSH_PITCH, 8.0f}, {J_RELB, 30.0f}, {J_LSH_PITCH, 8.0f}, {J_LELB, 30.0f} });
}

// ---------------------------------------------------------------------------
//  Keyframes: SAQUE (contacto en t = 1.55 s)
// ---------------------------------------------------------------------------
const std::vector<PoseKey>& Anim::serveTrack()
{
    static std::vector<PoseKey> k;
    if (!k.empty()) return k;
    Pose stance = K(Pose(), { {J_LEAN, 3.0f}, {J_TORSO_PITCH, 4.0f}, {J_TORSO_YAW, -38.0f},
                              {J_RHIP, 14.0f}, {J_RKNEE, 26.0f}, {J_LHIP, 14.0f}, {J_LKNEE, 26.0f},
                              {J_RHIP_ROLL, 9.0f}, {J_LHIP_ROLL, 9.0f},
                              {J_RSH_PITCH, 24.0f}, {J_RELB, 48.0f}, {J_RWR_PITCH, 8.0f}, {J_RSH_ROLL, 8.0f},
                              {J_LSH_PITCH, 38.0f}, {J_LELB, 55.0f}, {J_LSH_ROLL, 5.0f}, {J_HEAD_PITCH, 0.0f} });
    k.push_back(key(0.00f, stance));
    k.push_back(key(0.40f, K(stance, { {J_RSH_PITCH, -10.0f}, {J_RELB, 22.0f}, {J_LSH_PITCH, 60.0f}, {J_RKNEE, 32.0f}, {J_LKNEE, 32.0f}, {J_TORSO_YAW, -42.0f} })));
    k.push_back(key(0.92f, K(stance, { {J_LSH_PITCH, 165.0f}, {J_LELB, 4.0f}, {J_LSH_ROLL, 14.0f},
                                       {J_RSH_PITCH, 30.0f}, {J_RSH_ROLL, 40.0f}, {J_RELB, 95.0f}, {J_RWR_PITCH, 0.0f},
                                       {J_RKNEE, 42.0f}, {J_LKNEE, 42.0f}, {J_RHIP, 20.0f}, {J_LHIP, 20.0f},
                                       {J_TORSO_PITCH, -4.0f}, {J_HEAD_PITCH, -14.0f}, {J_TORSO_YAW, -40.0f} })));
    k.push_back(key(1.18f, K(stance, { {J_LSH_PITCH, 170.0f}, {J_LELB, 4.0f}, {J_LSH_ROLL, 18.0f},
                                       {J_RSH_PITCH, 70.0f}, {J_RSH_ROLL, 78.0f}, {J_RELB, 105.0f}, {J_RWR_PITCH, -35.0f},
                                       {J_RKNEE, 62.0f}, {J_LKNEE, 62.0f}, {J_RHIP, 30.0f}, {J_LHIP, 30.0f},
                                       {J_TORSO_PITCH, -14.0f}, {J_LEAN, -2.0f}, {J_HEAD_PITCH, -26.0f}, {J_TORSO_YAW, -34.0f} })));
    k.push_back(key(1.38f, K(stance, { {J_LSH_PITCH, 150.0f}, {J_LELB, 12.0f}, {J_LSH_ROLL, 12.0f},
                                       {J_RSH_PITCH, 100.0f}, {J_RSH_ROLL, 62.0f}, {J_RELB, 125.0f}, {J_RWR_PITCH, -55.0f},
                                       {J_RKNEE, 40.0f}, {J_LKNEE, 40.0f}, {J_RHIP, 20.0f}, {J_LHIP, 20.0f},
                                       {J_TORSO_PITCH, -10.0f}, {J_HEAD_PITCH, -30.0f}, {J_TORSO_YAW, -12.0f}, {J_JUMP, 0.03f} })));
    k.push_back(key(1.55f, K(stance, { {J_LSH_PITCH, 60.0f}, {J_LELB, 70.0f}, {J_LSH_ROLL, 10.0f},
                                       {J_RSH_PITCH, 168.0f}, {J_RSH_ROLL, 4.0f}, {J_RELB, 6.0f}, {J_RWR_PITCH, 12.0f},
                                       {J_RKNEE, 6.0f}, {J_LKNEE, 8.0f}, {J_RHIP, 4.0f}, {J_LHIP, 6.0f},
                                       {J_TORSO_PITCH, 10.0f}, {J_LEAN, 4.0f}, {J_HEAD_PITCH, -20.0f}, {J_TORSO_YAW, 14.0f}, {J_JUMP, 0.16f} })));
    k.push_back(key(1.85f, K(stance, { {J_LSH_PITCH, 20.0f}, {J_LELB, 70.0f},
                                       {J_RSH_PITCH, 70.0f}, {J_RSH_YAW, 52.0f}, {J_RSH_ROLL, 10.0f}, {J_RELB, 45.0f}, {J_RWR_PITCH, 12.0f},
                                       {J_RKNEE, 36.0f}, {J_LKNEE, 30.0f}, {J_RHIP, 28.0f}, {J_LHIP, 22.0f},
                                       {J_TORSO_PITCH, 28.0f}, {J_LEAN, 10.0f}, {J_HEAD_PITCH, 12.0f}, {J_TORSO_YAW, 58.0f}, {J_JUMP, 0.02f} })));
    k.push_back(key(2.30f, K(stance, { {J_RSH_PITCH, 30.0f}, {J_RSH_YAW, 25.0f}, {J_RELB, 62.0f},
                                       {J_LSH_PITCH, 30.0f}, {J_LELB, 65.0f},
                                       {J_RKNEE, 32.0f}, {J_LKNEE, 32.0f}, {J_RHIP, 18.0f}, {J_LHIP, 18.0f},
                                       {J_TORSO_PITCH, 12.0f}, {J_TORSO_YAW, 25.0f} })));
    k.push_back(key(3.00f, ready()));
    return k;
}

// ---------------------------------------------------------------------------
//  Keyframes: DERECHA (forehand) - contacto en t = 0.46 s
// ---------------------------------------------------------------------------
const std::vector<PoseKey>& Anim::forehandTrack()
{
    static std::vector<PoseKey> k;
    if (!k.empty()) return k;
    Pose r = ready();
    k.push_back(key(0.00f, r));
    k.push_back(key(0.24f, K(r, { {J_TORSO_YAW, -52.0f}, {J_LEAN, 8.0f}, {J_TORSO_PITCH, 12.0f},
                                  {J_RSH_PITCH, -22.0f}, {J_RSH_ROLL, 28.0f}, {J_RELB, 78.0f}, {J_RWR_PITCH, 35.0f},
                                  {J_LSH_PITCH, 78.0f}, {J_LELB, 14.0f}, {J_LSH_ROLL, -10.0f},
                                  {J_RHIP, 30.0f}, {J_RKNEE, 54.0f}, {J_LHIP, 18.0f}, {J_LKNEE, 40.0f}, {J_HEAD_YAW, 25.0f} })));
    k.push_back(key(0.46f, K(r, { {J_TORSO_YAW, 4.0f}, {J_LEAN, 10.0f}, {J_TORSO_PITCH, 14.0f},
                                  {J_RSH_PITCH, 74.0f}, {J_RSH_ROLL, 18.0f}, {J_RSH_YAW, -4.0f}, {J_RELB, 24.0f}, {J_RWR_PITCH, 68.0f},
                                  {J_LSH_PITCH, -15.0f}, {J_LELB, 40.0f}, {J_LSH_ROLL, 42.0f},
                                  {J_RHIP, 14.0f}, {J_RKNEE, 28.0f}, {J_LHIP, 38.0f}, {J_LKNEE, 52.0f}, {J_HEAD_YAW, 0.0f} })));
    k.push_back(key(0.74f, K(r, { {J_TORSO_YAW, 70.0f}, {J_LEAN, 6.0f}, {J_TORSO_PITCH, 8.0f},
                                  {J_RSH_PITCH, 118.0f}, {J_RSH_YAW, 78.0f}, {J_RSH_ROLL, 8.0f}, {J_RELB, 96.0f}, {J_RWR_PITCH, 30.0f},
                                  {J_LSH_PITCH, 10.0f}, {J_LELB, 60.0f}, {J_LSH_ROLL, 30.0f},
                                  {J_RHIP, 12.0f}, {J_RKNEE, 22.0f}, {J_LHIP, 30.0f}, {J_LKNEE, 36.0f}, {J_HEAD_YAW, -20.0f} })));
    k.push_back(key(1.25f, r));
    return k;
}

// ---------------------------------------------------------------------------
//  Keyframes: ESTIRADA (no alcanza la pelota)
// ---------------------------------------------------------------------------
const std::vector<PoseKey>& Anim::lungeTrack()
{
    static std::vector<PoseKey> k;
    if (!k.empty()) return k;
    Pose r = ready();
    k.push_back(key(0.00f, r));
    k.push_back(key(0.20f, K(r, { {J_TORSO_YAW, -40.0f}, {J_RSH_PITCH, 5.0f}, {J_RSH_ROLL, 40.0f}, {J_RELB, 70.0f}, {J_RWR_PITCH, 30.0f},
                                  {J_LEAN, 12.0f}, {J_RHIP, 34.0f}, {J_RKNEE, 60.0f} })));
    k.push_back(key(0.45f, K(r, { {J_TORSO_YAW, -4.0f}, {J_LEAN, 20.0f}, {J_TORSO_PITCH, 18.0f},
                                  {J_RSH_PITCH, 70.0f}, {J_RSH_ROLL, 50.0f}, {J_RELB, 10.0f}, {J_RWR_PITCH, 55.0f},
                                  {J_LSH_PITCH, -25.0f}, {J_LSH_ROLL, 55.0f}, {J_LELB, 20.0f},
                                  {J_RHIP, -14.0f}, {J_RKNEE, 14.0f}, {J_LHIP, 62.0f}, {J_LKNEE, 74.0f},
                                  {J_RHIP_ROLL, 16.0f}, {J_LHIP_ROLL, 14.0f} })));
    k.push_back(key(0.75f, K(r, { {J_TORSO_YAW, 24.0f}, {J_LEAN, 12.0f}, {J_TORSO_PITCH, 14.0f},
                                  {J_RSH_PITCH, 55.0f}, {J_RSH_ROLL, 40.0f}, {J_RELB, 30.0f}, {J_RWR_PITCH, 40.0f},
                                  {J_RHIP, 4.0f}, {J_RKNEE, 20.0f}, {J_LHIP, 50.0f}, {J_LKNEE, 60.0f}, {J_HEAD_PITCH, 14.0f} })));
    k.push_back(key(1.10f, K(r, { {J_TORSO_PITCH, 14.0f}, {J_HEAD_PITCH, 18.0f}, {J_RSH_PITCH, 18.0f}, {J_RELB, 40.0f}, {J_LSH_PITCH, 14.0f}, {J_LELB, 40.0f} })));
    return k;
}

// ---------------------------------------------------------------------------
//  Keyframes: FESTEJO del ganador del punto
// ---------------------------------------------------------------------------
const std::vector<PoseKey>& Anim::celebrateTrack()
{
    static std::vector<PoseKey> k;
    if (!k.empty()) return k;
    Pose r = ready();
    Pose arms = K(Pose(), { {J_RSH_PITCH, 165.0f}, {J_RELB, 25.0f}, {J_LSH_PITCH, 165.0f}, {J_LELB, 25.0f}, {J_RSH_ROLL, 20.0f}, {J_LSH_ROLL, 20.0f},
                            {J_HEAD_PITCH, -18.0f}, {J_TORSO_PITCH, -8.0f}, {J_RHIP, 4.0f}, {J_LHIP, 4.0f}, {J_RKNEE, 8.0f}, {J_LKNEE, 8.0f}, {J_JUMP, 0.20f} });
    k.push_back(key(0.00f, r));
    k.push_back(key(0.28f, K(r, { {J_RKNEE, 70.0f}, {J_LKNEE, 70.0f}, {J_RHIP, 38.0f}, {J_LHIP, 38.0f}, {J_RSH_PITCH, 10.0f}, {J_LSH_PITCH, 10.0f}, {J_TORSO_PITCH, 18.0f} })));
    k.push_back(key(0.62f, arms));
    k.push_back(key(1.00f, K(Pose(), { {J_RSH_PITCH, 100.0f}, {J_RELB, 135.0f}, {J_RWR_PITCH, 10.0f}, {J_LSH_PITCH, 15.0f}, {J_LELB, 70.0f},
                                       {J_TORSO_YAW, -22.0f}, {J_TORSO_PITCH, 10.0f}, {J_HEAD_PITCH, -6.0f},
                                       {J_RHIP, 26.0f}, {J_RKNEE, 46.0f}, {J_LHIP, 26.0f}, {J_LKNEE, 46.0f}, {J_RHIP_ROLL, 8.0f}, {J_LHIP_ROLL, 8.0f} })));
    k.push_back(key(1.45f, K(Pose(), { {J_RSH_PITCH, 150.0f}, {J_RELB, 18.0f}, {J_RWR_PITCH, 0.0f}, {J_LSH_PITCH, 20.0f}, {J_LELB, 60.0f},
                                       {J_TORSO_YAW, 8.0f}, {J_HEAD_PITCH, -10.0f}, {J_RHIP, 8.0f}, {J_LHIP, 8.0f}, {J_RKNEE, 14.0f}, {J_LKNEE, 14.0f} })));
    k.push_back(key(1.90f, K(r, { {J_RSH_PITCH, 140.0f}, {J_RELB, 24.0f}, {J_TORSO_YAW, -10.0f} })));
    k.push_back(key(2.60f, r));
    return k;
}

// ---------------------------------------------------------------------------
//  Keyframes: DESANIMO del perdedor
// ---------------------------------------------------------------------------
const std::vector<PoseKey>& Anim::dejectTrack()
{
    static std::vector<PoseKey> k;
    if (!k.empty()) return k;
    Pose r = ready();
    Pose slump = K(Pose(), { {J_TORSO_PITCH, 18.0f}, {J_LEAN, 6.0f}, {J_HEAD_PITCH, 26.0f}, {J_RSH_PITCH, 4.0f}, {J_RELB, 16.0f}, {J_LSH_PITCH, 6.0f}, {J_LELB, 14.0f},
                             {J_RHIP, 8.0f}, {J_RKNEE, 14.0f}, {J_LHIP, 8.0f}, {J_LKNEE, 14.0f}, {J_RHIP_ROLL, 3.0f}, {J_LHIP_ROLL, 3.0f} });
    k.push_back(key(0.00f, r));
    k.push_back(key(0.50f, slump));
    k.push_back(key(0.90f, K(slump, { {J_HEAD_YAW, -26.0f} })));
    k.push_back(key(1.30f, K(slump, { {J_HEAD_YAW, 26.0f} })));
    k.push_back(key(1.70f, K(slump, { {J_HEAD_YAW, 0.0f}, {J_HEAD_PITCH, 18.0f} })));
    k.push_back(key(2.20f, r));
    return k;
}

// ---------------------------------------------------------------------------
//  Keyframes: RECOGER la pelota (recogepelotas)
// ---------------------------------------------------------------------------
const std::vector<PoseKey>& Anim::pickTrack()
{
    static std::vector<PoseKey> k;
    if (!k.empty()) return k;
    Pose a = kidReady();
    Pose down = K(Pose(), { {J_LEAN, 30.0f}, {J_TORSO_PITCH, 25.0f}, {J_HEAD_PITCH, 10.0f},
                            {J_RHIP, 70.0f}, {J_RKNEE, 100.0f}, {J_LHIP, 40.0f}, {J_LKNEE, 60.0f},
                            {J_RSH_PITCH, 72.0f}, {J_RELB, 12.0f}, {J_LSH_PITCH, 10.0f}, {J_LELB, 30.0f}, {J_RHIP_ROLL, 8.0f}, {J_LHIP_ROLL, 8.0f} });
    k.push_back(key(0.00f, a));
    k.push_back(key(0.40f, down));
    k.push_back(key(0.60f, down));
    k.push_back(key(1.00f, a));
    return k;
}
