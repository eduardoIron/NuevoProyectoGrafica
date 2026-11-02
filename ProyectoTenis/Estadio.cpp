#include "Estadio.h"
#include "Config.h"
#include <iostream>

using namespace Cfg;

typedef glm::vec2 V2;
typedef glm::vec3 V3;
typedef glm::vec4 V4;
typedef glm::mat4 M4;

namespace
{
    M4 T(float x, float y, float z) { return glm::translate(M4(1.0f), V3(x, y, z)); }
    M4 T(const V3& v) { return glm::translate(M4(1.0f), v); }
    M4 RY(float deg) { return glm::rotate(M4(1.0f), U::rad(deg), V3(0, 1, 0)); }
    M4 RX(float deg) { return glm::rotate(M4(1.0f), U::rad(deg), V3(1, 0, 0)); }
    M4 S(float x, float y, float z) { return glm::scale(M4(1.0f), V3(x, y, z)); }
    V3 W(const M4& m, const V3& p) { return V3(m * V4(p, 1.0f)); }

    // color fijo (alfa 0 = no se tiñe en el shader del público)
    V4 C(float r, float g, float b) { return V4(r, g, b, 0.0f); }

    // Letrero: una fila del atlas "signs"
    void addSign(MeshBuilder& mb, const M4& xf, float w, float h, int row)
    {
        unsigned int base = static_cast<unsigned int>(mb.v.size());
        float hx = w * 0.5f, hy = h * 0.5f;
        float v0 = static_cast<float>(row) / Textures::SIGN_ROWS, v1 = static_cast<float>(row + 1) / Textures::SIGN_ROWS;
        V3 n = glm::normalize(glm::mat3(xf) * V3(0, 0, 1));
        const V3 P[4] = { V3(-hx, -hy, 0), V3(hx, -hy, 0), V3(hx, hy, 0), V3(-hx, hy, 0) };
        const V2 UV[4] = { V2(0, v1), V2(1, v1), V2(1, v0), V2(0, v0) };
        for (int i = 0; i < 4; ++i)
        {
            Vertex vt;
            vt.pos = W(xf, P[i]); vt.nrm = n; vt.uv = UV[i]; vt.col = V4(1.0f);
            mb.v.push_back(vt);
        }
        unsigned int ii[6] = { 0, 1, 2, 0, 2, 3 };
        for (int i = 0; i < 6; ++i) mb.idx.push_back(base + ii[i]);
    }

    V3 seatColor(int palette, int k, int r)
    {
        static const V3 pal[4][3] = {
            { V3(0.10f, 0.26f, 0.78f), V3(0.92f, 0.92f, 0.95f), V3(0.05f, 0.13f, 0.48f) },
            { V3(0.82f, 0.12f, 0.12f), V3(0.94f, 0.94f, 0.94f), V3(0.50f, 0.05f, 0.06f) },
            { V3(0.08f, 0.56f, 0.26f), V3(0.92f, 0.92f, 0.95f), V3(0.04f, 0.30f, 0.14f) },
            { V3(0.96f, 0.72f, 0.10f), V3(0.16f, 0.16f, 0.19f), V3(0.88f, 0.42f, 0.06f) } };
        int idx = (((k / 6) - (r / 3)) % 3 + 3) % 3;
        return pal[palette][idx];
    }
}

void Estadio::addBatch(const MeshBuilder& mb, const Material& m)
{
    if (mb.v.empty()) return;
    Batch b;
    b.mesh = mb.build();
    b.mat = m;
    batches_.push_back(b);
}

// ---------------------------------------------------------------------------
//  Altura del terreno (para la cámara "espectador")
// ---------------------------------------------------------------------------
float Estadio::groundHeight(float x, float z) const
{
    float ax = std::fabs(x), az = std::fabs(z);
    if (z < 0.0f && ax < TUNNEL_HALF_W && z > TUNNEL_Z1) return 0.0f;             // dentro del túnel
    if (ax >= SIDE_X0 && az <= SIDE_HALF_Z)
    {
        int r = static_cast<int>((ax - SIDE_X0) / ROW_DEPTH);
        return r < SIDE_ROWS ? (r + 1) * ROW_RISE : 0.0f;
    }
    if (az >= END_Z0 && ax <= END_HALF_X)
    {
        int r = static_cast<int>((az - END_Z0) / ROW_DEPTH);
        return r < END_ROWS ? (r + 1) * ROW_RISE : 0.0f;
    }
    return 0.0f;
}

// ---------------------------------------------------------------------------
//  Gradas: peldaños de concreto, asientos y público
// ---------------------------------------------------------------------------
void Estadio::buildStand(MeshBuilder& conc, MeshBuilder& seats, std::vector<float>& crowd,
                         const V3& origin, float yawDeg, float halfLen, int rows, bool tunnelCut,
                         int palette, uint32_t seed)
{
    M4 X = T(origin) * RY(yawDeg);
    const float gapW = TUNNEL_HALF_W + TUNNEL_WALL;
    const float tunnelTop = TUNNEL_H + 0.30f;
    U::Rng rng(seed);

    // barrera frontal
    conc.box(X * T(0, 0.45f, -0.18f), V3(2.0f * halfLen, 0.9f, 0.20f), V2(2.0f), C(0.55f, 0.58f, 0.62f));

    for (int r = 0; r < rows; ++r)
    {
        float top = (r + 1) * ROW_RISE;
        float z0 = r * ROW_DEPTH;
        bool cutRow = tunnelCut && top <= tunnelTop;
        bool slabRow = tunnelCut && top > tunnelTop;

        auto step = [&](float xa, float xb, float y0)
        {
            conc.box(X * T((xa + xb) * 0.5f, (y0 + top) * 0.5f, z0 + ROW_DEPTH * 0.5f), V3(xb - xa, top - y0, ROW_DEPTH), V2(2.0f, 2.0f));
        };
        if (cutRow)
        {
            step(-halfLen, -gapW, 0.0f);
            step(gapW, halfLen, 0.0f);
        }
        else if (slabRow)
        {
            step(-halfLen, -gapW, 0.0f);
            step(gapW, halfLen, 0.0f);
            step(-gapW, gapW, tunnelTop - 0.30f);
        }
        else
            step(-halfLen, halfLen, 0.0f);

        // asientos (3 piezas por asiento) y espectadores
        int k = 0;
        for (float x = -halfLen + 0.55f; x < halfLen - 0.45f; x += 0.52f, ++k)
        {
            if (k % 11 == 10) continue;                                   // pasillo
            if ((cutRow || slabRow) && std::fabs(x) < gapW + 0.30f && top <= tunnelTop + 0.2f) continue;
            if (slabRow && std::fabs(x) < gapW + 0.30f && top <= tunnelTop + 0.5f) { }
            V3 sc = seatColor(palette, k, r);
            V4 col(sc, 1.0f);
            seats.box(X * T(x, top + 0.42f, z0 + 0.58f), V3(0.46f, 0.06f, 0.42f), V2(1.0f, 0.5f), col);
            seats.box(X * T(x, top + 0.68f, z0 + 0.81f), V3(0.46f, 0.46f, 0.06f), V2(1.0f, 0.5f), col);
            seats.box(X * T(x, top + 0.20f, z0 + 0.58f), V3(0.10f, 0.40f, 0.10f), V2(1.0f, 1.0f), V4(0.2f, 0.2f, 0.22f, 1.0f));

            if (rng.chance(0.86f))
            {
                V3 wp = W(X, V3(x, top + 0.45f, z0 + 0.58f));
                float ang = std::atan2(wp.z, wp.x);
                // paleta de camisas (colores del equipo + variados)
                static const V3 shirts[10] = {
                    V3(0.90f, 0.90f, 0.92f), V3(0.10f, 0.25f, 0.75f), V3(0.80f, 0.12f, 0.12f), V3(0.95f, 0.75f, 0.10f),
                    V3(0.10f, 0.50f, 0.28f), V3(0.15f, 0.15f, 0.18f), V3(0.90f, 0.45f, 0.10f), V3(0.60f, 0.20f, 0.60f),
                    V3(0.30f, 0.65f, 0.85f), V3(0.75f, 0.75f, 0.50f) };
                V3 sh = shirts[rng.irange(0, 9)];
                float yaw = U::rad(yawDeg + 180.0f);
                float inst[8] = { wp.x, wp.y, wp.z, ang / 0.30f, sh.x, sh.y, sh.z, yaw };
                crowd.insert(crowd.end(), inst, inst + 8);
            }
        }
    }
    // muro trasero
    float total = rows * ROW_DEPTH;
    float topAll = rows * ROW_RISE;
    conc.box(X * T(0, (topAll + 1.2f) * 0.5f, total + 0.15f), V3(2.0f * halfLen, topAll + 1.2f, 0.30f), V2(2.0f), C(0.70f, 0.72f, 0.76f));
}

// ---------------------------------------------------------------------------
//  Piso, muro perimetral con publicidad
// ---------------------------------------------------------------------------
void Estadio::buildCourtAndWalls(MeshBuilder& conc, MeshBuilder& ads, MeshBuilder& props)
{
    { MeshBuilder f; f.plane(T(0, 0, 0), 2.0f * FLOOR_HX, 2.0f * FLOOR_HZ, V2(3.0f, 3.0f)); courtMesh_ = f.build(); }

    const float t = 0.25f, H = WALL_H;
    V4 wallCol = C(0.16f, 0.36f, 0.24f);
    const float gap = TUNNEL_HALF_W + TUNNEL_WALL;
    // concreto del muro
    conc.boxAt(V3(FLOOR_HX, 0, -FLOOR_HZ - t), V3(FLOOR_HX + t, H, FLOOR_HZ + t), V2(2.0f), wallCol);      // este
    conc.boxAt(V3(-FLOOR_HX - t, 0, -FLOOR_HZ - t), V3(-FLOOR_HX, H, FLOOR_HZ + t), V2(2.0f), wallCol);    // oeste
    conc.boxAt(V3(-FLOOR_HX, 0, FLOOR_HZ), V3(FLOOR_HX, H, FLOOR_HZ + t), V2(2.0f), wallCol);              // norte
    conc.boxAt(V3(-FLOOR_HX, 0, -FLOOR_HZ - t), V3(-gap, H, -FLOOR_HZ), V2(2.0f), wallCol);                // sur (izq.)
    conc.boxAt(V3(gap, 0, -FLOOR_HZ - t), V3(FLOOR_HX, H, -FLOOR_HZ), V2(2.0f), wallCol);                   // sur (der.)

    // paneles de publicidad pegados a la cara interior
    const float adH = 0.86f, adY = 0.16f + adH * 0.5f, texLen = 28.8f;
    auto panel = [&](float cx, float cz, float len, float faceYawDeg, float uOffset)
    {
        float u0 = uOffset / texLen;
        // quad centrado, mira a +Z local; uvMax en x = len/texLen
        MeshBuilder q;
        q.quad(T(0, 0, 0), len, adH, V2(len / texLen, 1.0f));
        for (size_t i = 0; i < q.v.size(); ++i) q.v[i].uv.x += u0;
        ads.append(q, T(cx, adY, cz) * RY(faceYawDeg));
    };
    panel(FLOOR_HX - 0.005f, 0.0f, 2.0f * FLOOR_HZ, -90.0f, 0.0f);          // este (mira a -X)
    panel(-FLOOR_HX + 0.005f, 0.0f, 2.0f * FLOOR_HZ, 90.0f, 7.2f);          // oeste (mira a +X)
    panel(0.0f, FLOOR_HZ - 0.005f, 2.0f * FLOOR_HX, 180.0f, 14.4f);         // norte (mira a -Z)
    panel(-(FLOOR_HX + gap) * 0.5f, -FLOOR_HZ + 0.005f, FLOOR_HX - gap, 0.0f, 21.6f);   // sur izq.
    panel((FLOOR_HX + gap) * 0.5f, -FLOOR_HZ + 0.005f, FLOOR_HX - gap, 0.0f, 3.6f);     // sur der.

    // remate oscuro del muro
    props.boxAt(V3(FLOOR_HX - 0.02f, H, -FLOOR_HZ - t), V3(FLOOR_HX + t, H + 0.04f, FLOOR_HZ + t), V2(1.0f), C(0.05f, 0.08f, 0.07f));
    props.boxAt(V3(-FLOOR_HX - t, H, -FLOOR_HZ - t), V3(-FLOOR_HX + 0.02f, H + 0.04f, FLOOR_HZ + t), V2(1.0f), C(0.05f, 0.08f, 0.07f));
    props.boxAt(V3(-FLOOR_HX, H, FLOOR_HZ - 0.02f), V3(FLOOR_HX, H + 0.04f, FLOOR_HZ + t), V2(1.0f), C(0.05f, 0.08f, 0.07f));
    props.boxAt(V3(-FLOOR_HX, H, -FLOOR_HZ - t), V3(-gap, H + 0.04f, -FLOOR_HZ + 0.02f), V2(1.0f), C(0.05f, 0.08f, 0.07f));
    props.boxAt(V3(gap, H, -FLOOR_HZ - t), V3(FLOOR_HX, H + 0.04f, -FLOOR_HZ + 0.02f), V2(1.0f), C(0.05f, 0.08f, 0.07f));
}

// ---------------------------------------------------------------------------
//  Túnel de vestidores
// ---------------------------------------------------------------------------
void Estadio::buildTunnel(MeshBuilder& conc, MeshBuilder& lamps, MeshBuilder& signs)
{
    const float hw = TUNNEL_HALF_W, wt = TUNNEL_WALL, h = TUNNEL_H;
    const float z0 = TUNNEL_Z0, z1 = TUNNEL_Z1;
    V4 wallC = C(0.78f, 0.78f, 0.80f), floorC = C(0.35f, 0.36f, 0.40f);

    conc.boxAt(V3(-hw, 0.0f, z1), V3(hw, 0.02f, z0), V2(2.0f), floorC);                         // piso
    conc.boxAt(V3(-hw - wt, 0.0f, z1 - wt), V3(-hw, h, z0), V2(2.0f), wallC);                   // pared izquierda
    conc.boxAt(V3(hw, 0.0f, z1 - wt), V3(hw + wt, h, z0), V2(2.0f), wallC);                     // pared derecha
    conc.boxAt(V3(-hw - wt, h, z1 - wt), V3(hw + wt, h + 0.30f, z0), V2(2.0f), wallC);          // techo
    conc.boxAt(V3(-hw, 0.0f, z1 - wt), V3(hw, h, z1), V2(2.0f), C(0.80f, 0.80f, 0.82f));        // fondo
    // dintel sobre la boca (para el letrero)
    conc.boxAt(V3(-hw - 0.6f, h + 0.30f, z0 - 0.35f), V3(hw + 0.6f, h + 1.10f, z0), V2(2.0f), C(0.12f, 0.28f, 0.20f));

    // lámparas del techo (tiras emisivas)
    for (int i = 0; i < 3; ++i)
    {
        float z = z0 - 3.2f - i * 3.2f;
        lamps.box(T(0.0f, h - 0.03f, z), V3(1.6f, 0.05f, 0.35f), V2(1.0f));
    }
    // puertas en el fondo
    for (int s = -1; s <= 1; s += 2)
    {
        float cx = s * 1.05f;
        conc.box(T(cx, 1.05f, z1 + 0.04f), V3(1.0f, 2.1f, 0.08f), V2(1.0f), C(0.30f, 0.20f, 0.12f));
        conc.box(T(cx - s * 0.36f, 1.0f, z1 + 0.10f), V3(0.05f, 0.12f, 0.06f), V2(1.0f), C(0.85f, 0.85f, 0.85f));   // manija
    }
    // zócalo
    conc.boxAt(V3(-hw, 0.0f, z1), V3(-hw + 0.04f, 0.25f, z0), V2(1.0f), C(0.15f, 0.15f, 0.17f));
    conc.boxAt(V3(hw - 0.04f, 0.0f, z1), V3(hw, 0.25f, z0), V2(1.0f), C(0.15f, 0.15f, 0.17f));

    // letreros (atlas "signs": 0 VESTIDORES, 1 LOCAL, 2 VISITA, 3 ENTRADA JUGADORES)
    addSign(signs, T(0.0f, 2.35f, z1 + 0.012f), 2.8f, 0.45f, 0);
    addSign(signs, T(-1.05f, 2.55f, z1 + 0.012f), 0.95f, 0.30f, 1);
    addSign(signs, T(1.05f, 2.55f, z1 + 0.012f), 0.95f, 0.30f, 2);
    addSign(signs, T(0.0f, h + 0.70f, z0 + 0.012f), 4.4f, 0.70f, 3);
}

// ---------------------------------------------------------------------------
//  Red y postes
// ---------------------------------------------------------------------------
void Estadio::buildNet(MeshBuilder& props)
{
    // postes
    for (int s = -1; s <= 1; s += 2)
    {
        props.cylinder(T(s * NET_POST_X, 0.0f, 0.0f), 0.055f, 0.045f, NET_H_POST + 0.02f, 12, true, V2(1.0f), C(0.10f, 0.10f, 0.12f));
        props.box(T(s * NET_POST_X, 0.01f, 0.0f), V3(0.30f, 0.02f, 0.30f), V2(1.0f), C(0.10f, 0.10f, 0.12f));
    }
    // cinta central
    props.box(T(0.0f, 0.46f, 0.0f), V3(0.05f, 0.88f, 0.012f), V2(1.0f), V4(0.95f, 0.95f, 0.95f, 0.0f));

    // malla con curvatura (más baja al centro)
    MeshBuilder net, band;
    const int nx = 64, ny = 8;
    auto topY = [&](float x) { float k = x / NET_POST_X; return NET_H_CENTER + (NET_H_POST - NET_H_CENTER) * k * k; };
    for (int j = 0; j <= ny; ++j)
        for (int i = 0; i <= nx; ++i)
        {
            float x = -NET_POST_X + 2.0f * NET_POST_X * i / nx;
            float y = U::lerp(0.03f, topY(x) - 0.04f, static_cast<float>(j) / ny);
            Vertex vt;
            vt.pos = V3(x, y, 0.0f); vt.nrm = V3(0, 0, 1); vt.uv = V2(x / 0.64f, y / 0.64f); vt.col = V4(1.0f);
            net.v.push_back(vt);
        }
    for (int j = 0; j < ny; ++j)
        for (int i = 0; i < nx; ++i)
        {
            unsigned int a = j * (nx + 1) + i, b = a + 1, c = b + (nx + 1), d = a + (nx + 1);
            unsigned int q[6] = { a, b, c, a, c, d };
            net.idx.insert(net.idx.end(), q, q + 6);
        }
    netMesh_ = net.build();

    // banda blanca superior
    for (int i = 0; i <= nx; ++i)
    {
        float x = -NET_POST_X + 2.0f * NET_POST_X * i / nx;
        for (int k = 0; k < 2; ++k)
        {
            Vertex vt;
            vt.pos = V3(x, k == 0 ? topY(x) - 0.06f : topY(x), 0.0f); vt.nrm = V3(0, 0, 1); vt.uv = V2(0, 0); vt.col = V4(1.0f);
            band.v.push_back(vt);
        }
    }
    for (int i = 0; i < nx; ++i)
    {
        unsigned int a = 2 * i, b = 2 * i + 1, c = 2 * i + 3, d = 2 * i + 2;
        unsigned int q[6] = { a, d, c, a, c, b };
        band.idx.insert(band.idx.end(), q, q + 6);
    }
    netBandMesh_ = band.build();
}

// ---------------------------------------------------------------------------
//  Silla del juez (estructura alta con escalera, micrófono y panel electrónico)
// ---------------------------------------------------------------------------
void Estadio::buildUmpireChair(MeshBuilder& props, MeshBuilder& screens)
{
    M4 X = T(UMPIRE_X, 0.0f, 0.0f) * RY(-90.0f);        // el frente (+Z local) mira a la cancha (-X)
    V4 metal = C(0.80f, 0.82f, 0.84f), dark = C(0.07f, 0.08f, 0.10f), blue = C(0.06f, 0.14f, 0.42f);

    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            props.bar(W(X, V3(sx * 0.44f, 0.0f, sz * 0.44f)), W(X, V3(sx * 0.40f, 2.30f, sz * 0.40f)), 0.035f, 8, metal);
    for (int lvl = 0; lvl < 2; ++lvl)
    {
        float y = lvl == 0 ? 0.75f : 1.55f;
        for (int s = -1; s <= 1; s += 2)
        {
            props.box(X * T(0.0f, y, s * 0.42f), V3(0.84f, 0.04f, 0.04f), V2(1.0f), metal);
            props.box(X * T(s * 0.42f, y, 0.0f), V3(0.04f, 0.04f, 0.84f), V2(1.0f), metal);
        }
    }
    props.box(X * T(0.0f, 2.30f, 0.0f), V3(0.98f, 0.08f, 0.98f), V2(1.0f), dark);              // plataforma
    props.box(X * T(0.0f, 2.37f, 0.0f), V3(0.66f, 0.06f, 0.66f), V2(1.0f), blue);               // cojín
    props.box(X * T(0.0f, 2.78f, -0.42f) * RX(-8.0f), V3(0.70f, 0.70f, 0.07f), V2(1.0f), blue); // respaldo
    for (int s = -1; s <= 1; s += 2)
    {
        props.box(X * T(s * 0.40f, 2.62f, -0.02f), V3(0.06f, 0.05f, 0.64f), V2(1.0f), dark);     // descansabrazos
        props.bar(W(X, V3(s * 0.40f, 2.34f, 0.22f)), W(X, V3(s * 0.40f, 2.62f, 0.22f)), 0.015f, 6, metal);
    }
    // estribo
    props.box(X * T(0.0f, 1.72f, 0.70f), V3(0.80f, 0.05f, 0.36f), V2(1.0f), dark);
    for (int s = -1; s <= 1; s += 2)
        props.bar(W(X, V3(s * 0.38f, 1.70f, 0.52f)), W(X, V3(s * 0.38f, 2.28f, 0.44f)), 0.02f, 6, metal);
    // escalera trasera
    for (int s = -1; s <= 1; s += 2)
        props.bar(W(X, V3(s * 0.32f, 0.0f, -1.45f)), W(X, V3(s * 0.32f, 2.28f, -0.50f)), 0.025f, 8, metal);
    for (int i = 1; i <= 6; ++i)
    {
        float t = i / 7.0f;
        V3 p = glm::mix(V3(0.0f, 0.0f, -1.45f), V3(0.0f, 2.28f, -0.50f), t);
        props.box(X * T(p) * RX(-22.0f), V3(0.64f, 0.035f, 0.07f), V2(1.0f), metal);
    }
    // micrófono con brazo
    props.bar(W(X, V3(0.40f, 2.64f, 0.28f)), W(X, V3(0.30f, 2.92f, 0.58f)), 0.012f, 6, dark);
    props.sphere(X * T(0.30f, 2.93f, 0.60f), 0.035f, 10, 8, dark);

    // mástil con panel electrónico (muestra el mismo marcador dinámico)
    props.bar(W(X, V3(-1.05f, 0.0f, 0.30f)), W(X, V3(-1.05f, 1.30f, 0.30f)), 0.03f, 8, metal);
    props.box(X * T(-1.05f, 1.62f, 0.27f), V3(1.00f, 0.54f, 0.07f), V2(1.0f), dark);
    screens.quad(X * T(-1.05f, 1.62f, 0.31f), 0.92f, 0.46f, V2(1.0f, 1.0f));
}

// ---------------------------------------------------------------------------
//  Zona de descanso de jugadores
// ---------------------------------------------------------------------------
void Estadio::buildRestZone(MeshBuilder& props, MeshBuilder& canopy)
{
    for (int k = 0; k < 2; ++k)
    {
        float z = (k == 0) ? 4.4f : -4.4f;
        V4 plastic = (k == 0) ? C(0.78f, 0.10f, 0.10f) : C(0.10f, 0.22f, 0.70f);
        M4 X = T(REST_X, 0.0f, z) * RY(90.0f);              // frente (+Z local) hacia la cancha (+X)
        chairXf_[k] = X;
        V4 leg = C(0.55f, 0.57f, 0.60f);

        props.box(X * T(0, 0.46f, 0), V3(0.52f, 0.06f, 0.50f), V2(1.0f), plastic);                        // asiento
        props.box(X * T(0, 0.80f, -0.23f) * RX(-8.0f), V3(0.52f, 0.62f, 0.05f), V2(1.0f), plastic);        // respaldo
        for (int sx = -1; sx <= 1; sx += 2)
            for (int sz = -1; sz <= 1; sz += 2)
                props.bar(W(X, V3(sx * 0.22f, 0, sz * 0.21f)), W(X, V3(sx * 0.22f, 0.45f, sz * 0.21f)), 0.017f, 6, leg);
        // mesita lateral (hacia el centro de la cancha en Z)
        float side = (k == 0) ? -1.0f : 1.0f;
        V3 tp(REST_X + 0.05f, 0.0f, z + side * 1.05f);
        props.cylinder(T(tp.x, 0.52f, tp.z), 0.30f, 0.30f, 0.04f, 20, true, V2(1.0f), C(0.92f, 0.92f, 0.94f));
        props.bar(V3(tp.x, 0.0f, tp.z), V3(tp.x, 0.53f, tp.z), 0.03f, 8, leg);
        props.cylinder(T(tp.x, 0.0f, tp.z), 0.18f, 0.18f, 0.02f, 16, true, V2(1.0f), leg);
        // hielera (azul con tapa blanca)
        V3 cp(REST_X - 0.45f, 0.0f, z - side * 0.95f);
        props.box(T(cp.x, 0.21f, cp.z), V3(0.62f, 0.42f, 0.40f), V2(1.0f), C(0.10f, 0.30f, 0.75f));
        props.box(T(cp.x, 0.45f, cp.z), V3(0.64f, 0.06f, 0.42f), V2(1.0f), C(0.92f, 0.92f, 0.95f));
        props.box(T(cp.x, 0.49f, cp.z), V3(0.20f, 0.03f, 0.08f), V2(1.0f), C(0.10f, 0.10f, 0.12f));

        // sombrilla: poste + dosel cónico
        V3 pp(REST_X - 0.85f, 0.0f, z);
        props.bar(pp, pp + V3(0, 2.45f, 0), 0.025f, 8, C(0.85f, 0.85f, 0.87f));
        props.cylinder(T(pp.x, 0.0f, pp.z), 0.20f, 0.20f, 0.03f, 16, true, V2(1.0f), leg);
        V4 cc = (k == 0) ? C(0.88f, 0.12f, 0.12f) : C(0.12f, 0.25f, 0.85f);
        canopy.cylinder(T(pp.x, 2.05f, pp.z), 1.55f, 0.05f, 0.45f, 16, false, V2(1.0f), cc);
        canopy.cylinder(T(pp.x, 2.05f, pp.z), 1.58f, 1.55f, 0.08f, 16, false, V2(1.0f), C(0.95f, 0.95f, 0.95f));
        canopy.sphere(T(pp.x, 2.52f, pp.z), 0.05f, 8, 6, C(0.85f, 0.85f, 0.87f));
    }
}

// ---------------------------------------------------------------------------
//  Torres de iluminación (4 esquinas)
// ---------------------------------------------------------------------------
void Estadio::buildTowers(MeshBuilder& props, MeshBuilder& lamps)
{
    anchors.towerLamps.clear();
    anchors.glowCenters.clear();
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
        {
            V3 base(sx * TOWER_X, 0.0f, sz * TOWER_Z);
            V3 top(base.x, TOWER_H, base.z);
            props.box(T(base.x, 0.3f, base.z), V3(2.4f, 0.6f, 2.4f), V2(1.0f), C(0.45f, 0.46f, 0.50f));
            props.cylinder(T(base), 0.55f, 0.32f, TOWER_H, 14, true, V2(1.0f), C(0.55f, 0.57f, 0.62f));
            // marco de la batería de lámparas, orientado al centro de la cancha
            V3 bankC = top + V3(0, 1.5f, 0);
            M4 face = glm::inverse(glm::lookAt(bankC, V3(-sx * 2.5f, 0.0f, -sz * 4.0f), V3(0, 1, 0)));   // el frente es -Z local
            props.box(face * T(0, 0, 0.35f), V3(7.4f, 5.4f, 0.30f), V2(1.0f), C(0.18f, 0.19f, 0.22f));
            props.box(T(base.x, TOWER_H - 0.3f, base.z), V3(1.2f, 0.4f, 1.2f), V2(1.0f), C(0.2f, 0.2f, 0.22f));
            for (int row = 0; row < 4; ++row)
                for (int col = 0; col < 3; ++col)
                {
                    float lx = (col - 1) * 2.2f, ly = (row - 1.5f) * 1.15f;
                    props.box(face * T(lx, ly, 0.05f), V3(1.7f, 1.0f, 0.30f), V2(1.0f), C(0.10f, 0.10f, 0.12f));
                    lamps.box(face * T(lx, ly, -0.14f), V3(1.45f, 0.82f, 0.06f), V2(1.0f));
                }
            anchors.towerLamps.push_back(bankC + (V3(-sx * 2.5f, 0.0f, -sz * 4.0f) - bankC) * 0.012f);
            for (int a = -1; a <= 1; a += 2)
                for (int b = -1; b <= 1; b += 2)
                    anchors.glowCenters.push_back(W(face, V3(a * 1.6f, b * 1.1f, -0.6f)));
        }
}

// ---------------------------------------------------------------------------
//  Marcador gigante (fondo norte)
// ---------------------------------------------------------------------------
void Estadio::buildScoreboard(MeshBuilder& props)
{
    const float z = END_Z0 + END_ROWS * ROW_DEPTH + 1.4f;     // detrás de las gradas norte
    const float cy = 9.6f;
    V4 steel = C(0.20f, 0.21f, 0.24f);
    for (int s = -1; s <= 1; s += 2)
    {
        props.cylinder(T(s * 5.2f, 0.0f, z + 0.3f), 0.30f, 0.26f, cy + 3.0f, 12, true, V2(1.0f), steel);
        props.box(T(s * 5.2f, 0.25f, z + 0.3f), V3(1.4f, 0.5f, 1.4f), V2(1.0f), C(0.45f, 0.46f, 0.5f));
    }
    props.box(T(0.0f, cy, z + 0.25f), V3(11.8f, 6.2f, 0.45f), V2(1.0f), steel);
    props.box(T(0.0f, cy + 3.3f, z + 0.1f), V3(12.0f, 0.3f, 0.7f), V2(1.0f), C(0.9f, 0.9f, 0.92f));
    anchors.scoreboardCenter = V3(0.0f, cy, z - 0.01f);

    MeshBuilder q;
    q.quad(T(0, 0, 0), 11.0f, 5.5f, V2(1.0f, 1.0f));
    MeshBuilder pos;
    pos.append(q, T(0.0f, cy, z - 0.001f) * RY(180.0f));
    scoreMesh_ = pos.build();
}

// ---------------------------------------------------------------------------
//  Cámaras de transmisión (utilería)
// ---------------------------------------------------------------------------
void Estadio::buildCameras(MeshBuilder& props)
{
    V4 body = C(0.10f, 0.10f, 0.12f), leg = C(0.30f, 0.30f, 0.33f), lens = C(0.02f, 0.02f, 0.03f);
    for (int i = 0; i < 2; ++i)
    {
        V3 toC = glm::normalize(V3(-anchors.tvCam[i].x, 0.0f, -anchors.tvCam[i].z));
        V3 p = anchors.tvCam[i] - toC * 1.6f - V3(0.0f, 0.75f, 0.0f);   // la utilería queda detrás/abajo del punto de vista
        float yaw = U::deg(std::atan2(toC.x, toC.z));
        M4 X = T(p) * RY(yaw);
        props.box(X * T(0, 0.10f, 0), V3(0.34f, 0.30f, 0.70f), V2(1.0f), body);
        props.cylinder(X * T(0, 0.10f, 0.35f) * RX(90.0f), 0.11f, 0.15f, 0.36f, 12, true, V2(1.0f), lens);
        props.box(X * T(0, 0.30f, -0.10f), V3(0.10f, 0.12f, 0.34f), V2(1.0f), body);
        for (int k = 0; k < 3; ++k)
        {
            float a = k * 2.0f * U::PI / 3.0f;
            props.bar(p + V3(0, -0.05f, 0), p + V3(std::cos(a) * 0.55f, -1.55f, std::sin(a) * 0.55f), 0.025f, 6, leg);
        }
    }
}

// ---------------------------------------------------------------------------
//  Espectador (una sola malla que se instancia ~2,600 veces)
// ---------------------------------------------------------------------------
void Estadio::buildSpectatorMesh()
{
    MeshBuilder m;
    V4 pants = C(0.13f, 0.16f, 0.29f), shirt(1.0f, 1.0f, 1.0f, 1.0f), skin(1.0f, 1.0f, 1.0f, 0.5f);
    V4 hair = C(0.10f, 0.07f, 0.05f), shoe = C(0.08f, 0.08f, 0.09f);
    for (int s = -1; s <= 1; s += 2)
    {
        m.box(T(s * 0.11f, 0.065f, 0.21f), V3(0.14f, 0.13f, 0.44f), V2(1.0f), pants);       // muslos
        m.box(T(s * 0.11f, -0.20f, 0.41f), V3(0.12f, 0.42f, 0.12f), V2(1.0f), pants);       // piernas
        m.box(T(s * 0.11f, -0.43f, 0.47f), V3(0.12f, 0.06f, 0.24f), V2(1.0f), shoe);        // zapatos
        m.box(T(s * 0.25f, 0.34f, 0.04f), V3(0.09f, 0.40f, 0.10f), V2(1.0f), shirt);        // brazos
    }
    m.box(T(0, 0.36f, 0.0f), V3(0.40f, 0.54f, 0.24f), V2(1.0f), shirt);                      // torso
    m.sphere(T(0, 0.80f, 0.02f), 0.115f, 10, 8, skin);                                        // cabeza
    m.sphere(T(0, 0.86f, -0.02f), 0.108f, 8, 6, hair);                                        // pelo
    crowdMesh_ = m.build();
}

// ---------------------------------------------------------------------------
//  Construcción general
// ---------------------------------------------------------------------------
void Estadio::build(const Textures& tex)
{
    tex_ = &tex;
    batches_.clear();

    // ----- materiales de la superficie de juego -----
    {
        Material m;
        m.tex = tex.clay; m.normalTex = tex.clayN; m.decalTex = tex.lines;
        m.specular = 0.03f; m.shininess = 8.0f; m.normalStrength = 1.0f;
        surfaceMat_[SURF_CLAY] = m;
        m.tex = tex.grass; m.normalTex = tex.grassN; m.specular = 0.02f; m.normalStrength = 1.3f;
        surfaceMat_[SURF_GRASS] = m;
        m.tex = tex.hard; m.normalTex = tex.hardN; m.specular = 0.30f; m.shininess = 48.0f; m.normalStrength = 0.6f;
        m.outsideTint = V3(1.25f, 1.35f, 0.50f);
        surfaceMat_[SURF_HARD] = m;
    }

    anchors.tvCam[0] = V3(-22.0f, 8.6f, 0.0f);
    anchors.tvCam[1] = V3(0.0f, 7.2f, 29.6f);
    anchors.netCenter = V3(0.0f, 0.9f, 0.0f);
    anchors.umpireSeat = V3(UMPIRE_X, 2.40f, 0.0f);
    anchors.umpireEye = V3(UMPIRE_X - 0.10f, 3.25f, 0.0f);
    anchors.tunnelMouth = V3(0.0f, 0.0f, TUNNEL_Z0);
    anchors.restChair[0] = V3(REST_X, 0.0f, 4.4f);
    anchors.restChair[1] = V3(REST_X, 0.0f, -4.4f);
    anchors.kidCorner[0] = V3(FLOOR_HX - 1.0f, 0.0f, FLOOR_HZ - 1.1f);
    anchors.kidCorner[1] = V3(-(FLOOR_HX - 1.0f), 0.0f, -(FLOOR_HZ - 1.1f));

    MeshBuilder bConc, bSeats, bAds, bProps, bCanopy, bLamps, bTunLamps, bSigns, bScreens, bAsphalt, bTunnelConc;
    std::vector<float> crowd;

    // ----- gradas -----
    float sideLen = SIDE_HALF_Z;
    buildStand(bConc, bSeats, crowd, V3(-SIDE_X0, 0, 0), -90.0f, sideLen, SIDE_ROWS, false, 0, 11u);   // oeste
    buildStand(bConc, bSeats, crowd, V3(SIDE_X0, 0, 0), 90.0f, sideLen, SIDE_ROWS, false, 1, 23u);     // este
    buildStand(bConc, bSeats, crowd, V3(0, 0, END_Z0), 0.0f, END_HALF_X, END_ROWS, false, 2, 37u);     // norte
    buildStand(bConc, bSeats, crowd, V3(0, 0, -END_Z0), 180.0f, END_HALF_X, END_ROWS, true, 3, 53u);   // sur (con túnel)
    crowdCount_ = static_cast<int>(crowd.size() / 8);

    buildCourtAndWalls(bConc, bAds, bProps);
    buildTunnel(bTunnelConc, bTunLamps, bSigns);
    buildNet(bProps);
    buildUmpireChair(bProps, bScreens);
    buildRestZone(bProps, bCanopy);
    buildTowers(bProps, bLamps);
    buildScoreboard(bProps);
    buildCameras(bProps);
    buildSpectatorMesh();
    crowdMesh_.setInstances(crowd);

    bAsphalt.plane(T(0, -0.05f, 0), 900.0f, 900.0f, V2(4.0f, 4.0f));

    // pelotas esparcidas en la cancha (se dibujan con el modelo de Assimp)
    ballSpots_.clear();
    ballSpots_.push_back(V3(-1.2f, 0, 13.6f));
    ballSpots_.push_back(V3(4.4f, 0, 14.4f));
    ballSpots_.push_back(V3(-3.8f, 0, -14.2f));
    ballSpots_.push_back(V3(2.6f, 0, -16.0f));
    ballSpots_.push_back(V3(7.6f, 0, 8.4f));
    ballSpots_.push_back(V3(-7.0f, 0, -9.6f));
    ballSpots_.push_back(V3(0.7f, 0, 3.9f));

    // luces puntuales del túnel
    anchors.points.clear();
    PointLight p1, p2;
    p1.pos = V3(0.0f, 2.4f, -22.0f); p1.color = V3(1.0f, 0.86f, 0.62f) * 2.4f;
    p2.pos = V3(0.0f, 2.4f, -27.0f); p2.color = V3(1.0f, 0.86f, 0.62f) * 2.4f;
    anchors.points.push_back(p1);
    anchors.points.push_back(p2);

    // ----- materiales y lotes -----
    Material conc;
    conc.tex = tex.concrete; conc.normalTex = tex.concreteN; conc.normalStrength = 0.7f;
    conc.specular = 0.04f; conc.shininess = 12.0f;
    addBatch(bConc, conc);
    addBatch(bTunnelConc, conc);

    Material seatM;
    seatM.tex = tex.seats; seatM.specular = 0.30f; seatM.shininess = 40.0f;
    addBatch(bSeats, seatM);

    Material adM;
    adM.tex = tex.ads; adM.specular = 0.25f; adM.shininess = 40.0f;
    addBatch(bAds, adM);

    Material propM(V3(1.0f), 0.35f, 48.0f);
    addBatch(bProps, propM);

    Material canopyM(V3(1.0f), 0.10f, 16.0f);
    canopyM.doubleSided = true;
    addBatch(bCanopy, canopyM);

    Material lampM(V3(1.0f), 0.0f, 8.0f);
    lampM.emissive = V3(7.0f, 6.4f, 5.2f);
    lampM.castShadow = false;
    addBatch(bLamps, lampM);
    Material tunLampM(V3(1.0f), 0.0f, 8.0f);
    tunLampM.emissive = V3(2.6f, 2.4f, 2.0f);
    addBatch(bTunLamps, tunLampM);

    Material signM(V3(1.0f), 0.10f, 16.0f);
    signM.tex = tex.signs; signM.selfIllum = 0.9f; signM.castShadow = false;
    addBatch(bSigns, signM);

    Material asph;
    asph.tex = tex.asphalt; asph.normalTex = tex.asphaltN; asph.normalStrength = 0.8f;
    asph.specular = 0.06f; asph.shininess = 16.0f; asph.castShadow = false;
    addBatch(bAsphalt, asph);

    // pantallas pequeñas (silla del juez): dinámicas
    {
        Material sm(V3(1.0f), 0.05f, 8.0f);
        sm.tex = tex.scoreboard; sm.selfIllum = 1.2f; sm.castShadow = false;
        scoreMat_ = sm;
        miniScoreMesh_ = bScreens.build();
    }

    netMat_ = Material(V3(1.0f), 0.05f, 8.0f);
    netMat_.tex = tex.net; netMat_.blend = true; netMat_.doubleSided = true; netMat_.alpha = 0.92f; netMat_.castShadow = false;
    bandMat_ = Material(V3(0.96f), 0.10f, 16.0f);
    bandMat_.doubleSided = true;

    std::cout << "Estadio construido: " << batches_.size() << " lotes, " << crowdCount_ << " espectadores." << std::endl;
}

// ---------------------------------------------------------------------------
//  Dibujo
// ---------------------------------------------------------------------------
void Estadio::drawOpaque(Renderer& r, const StadiumDraw& d, const Assets& A)
{
    const M4 I(1.0f);
    r.draw(courtMesh_, I, surfaceMat_[d.surface]);
    for (size_t i = 0; i < batches_.size(); ++i) r.draw(batches_[i].mesh, I, batches_[i].mat);
    r.draw(netBandMesh_, I, bandMat_);

    // pantallas del marcador (dinámicas, con un pequeño "pulso" cuando cambia el marcador)
    Material sm = scoreMat_;
    sm.selfIllum = 1.15f + 0.8f * d.scorePulse;
    float k = 1.0f + 0.035f * d.scorePulse;
    V3 c = anchors.scoreboardCenter;
    M4 pulse = T(c) * S(k, k, 1.0f) * T(-c);
    r.draw(scoreMesh_, pulse, sm);
    r.draw(miniScoreMesh_, I, sm);

    // público
    r.drawCrowd(crowdMesh_, crowdCount_, d.time, d.cheer, d.waveOn, d.waveTime);

    // --- utilería cargada con Assimp ---
    for (size_t i = 0; i < ballSpots_.size(); ++i)
        r.drawModel(A.ball, T(ballSpots_[i]) * RY(i * 57.0f));
    // raqueta tirada en la cancha
    r.drawModel(A.racket, T(-2.4f, 0.011f, -9.6f) * RY(33.0f) * RX(-90.0f));
    for (int k = 0; k < 2; ++k)
    {
        const M4& X = chairXf_[k];
        float side = (k == 0) ? -1.0f : 1.0f;
        float z = (k == 0) ? 4.4f : -4.4f;
        // toalla sobre el respaldo de la silla
        r.drawModel(A.towel, X * T(0.0f, 1.08f, -0.26f), k == 0 ? V3(1.0f) : V3(0.85f, 0.95f, 1.0f));
        // botellas sobre la mesita
        r.drawModel(A.bottle, T(REST_X + 0.05f - 0.10f, 0.56f, z + side * 1.05f + 0.06f));
        r.drawModel(A.bottle, T(REST_X + 0.05f + 0.12f, 0.56f, z + side * 1.05f - 0.08f), V3(0.85f, 1.0f, 0.85f));
        // botella junto a la hielera
        r.drawModel(A.bottle, T(REST_X - 0.15f, 0.0f, z - side * 0.55f));
        // raqueta de repuesto sobre la mesa
        r.drawModel(A.racket, T(REST_X + 0.12f, 0.575f, z + side * 1.05f) * RY(70.0f + k * 40.0f) * RX(-90.0f) * T(0.0f, -0.25f, 0.0f));
    }
}

void Estadio::drawTransparent(Renderer& r, const StadiumDraw& d)
{
    const M4 I(1.0f);
    r.draw(netMesh_, I, netMat_);
    if (d.floodLevel > 0.02f)
        r.drawGlow(anchors.glowCenters, 3.2f, V3(1.0f, 0.92f, 0.78f) * (0.55f * d.floodLevel));
}
