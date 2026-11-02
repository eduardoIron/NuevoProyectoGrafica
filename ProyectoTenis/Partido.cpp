#include "Partido.h"
#include "Config.h"
#include <algorithm>
#include <cstdio>

using namespace Cfg;

typedef glm::vec2 V2;
typedef glm::vec3 V3;
typedef glm::vec4 V4;
typedef glm::mat4 M4;

namespace
{
    // ---------------- configuración editable ----------------
    const char* NAMES[2] = { "ROJAS", "GARCIA" };
    const float CAT_FACING_OFFSET = 0.0f;       // giro para que el gato mire hacia donde camina
    const float DOG_FACING_OFFSET = 0.0f;

    M4 T(float x, float y, float z) { return glm::translate(M4(1.0f), V3(x, y, z)); }
    M4 RY(float d) { return glm::rotate(M4(1.0f), U::rad(d), V3(0, 1, 0)); }
    M4 RX(float d) { return glm::rotate(M4(1.0f), U::rad(d), V3(1, 0, 0)); }
    M4 S(float s) { return glm::scale(M4(1.0f), V3(s)); }

    V3 rotY(const V3& v, float deg)
    {
        float c = std::cos(U::rad(deg)), s = std::sin(U::rad(deg));
        return V3(v.x * c + v.z * s, v.y, -v.x * s + v.z * c);
    }
    float yawOf(int player) { return player == 0 ? 180.0f : 0.0f; }        // el jugador 0 está al norte (z>0) y mira a -Z
    V2 xz(const V3& v) { return V2(v.x, v.z); }

    // altura del arco para que la pelota pase la red con holgura
    float arcHeight(const V3& p0, const V3& p1, float minClear, float baseH)
    {
        if (p0.z * p1.z >= 0.0f) return baseH;
        float u = p0.z / (p0.z - p1.z);
        float yl = p0.y * (1.0f - u) + p1.y * u;
        float h = (minClear - yl) / (4.0f * u * (1.0f - u));
        return std::max(h, baseH);
    }

    V3 contactOffset(const std::vector<PoseKey>& track, float t)
    {
        Character c;
        c.pos = V3(0.0f);
        c.yaw = 0.0f;
        Pose p = Anim::evalTrack(track, t);
        Anim::flatFeet(p);
        c.pose = p;
        return c.racketCenter(c.skeleton());
    }
}

// ===========================================================================
//  Marcador
// ===========================================================================
void Score::reset()
{
    for (int i = 0; i < 2; ++i) { pts[i] = 0; games[i] = 0; }
    for (int k = 0; k < 3; ++k) { sets[k][0] = 0; sets[k][1] = 0; }
    setNo = 0; server = 0; over = false; winner = -1;
}

int Score::addPoint(int p)
{
    if (over) return 0;
    int o = 1 - p;
    pts[p]++;
    if (pts[p] >= 4 && pts[p] - pts[o] >= 2)
    {
        pts[0] = pts[1] = 0;
        games[p]++;
        server = 1 - server;
        if ((games[p] >= 6 && games[p] - games[o] >= 2) || games[p] == 7)
        {
            sets[setNo][0] = games[0]; sets[setNo][1] = games[1];
            games[0] = games[1] = 0;
            setNo++;
            int w[2] = { 0, 0 };
            for (int k = 0; k < setNo; ++k) w[sets[k][0] > sets[k][1] ? 0 : 1]++;
            if (w[0] == 2 || w[1] == 2 || setNo >= 3) { over = true; winner = w[0] > w[1] ? 0 : 1; return 3; }
            return 2;
        }
        return 1;
    }
    return 0;
}

std::string Score::ptsText(int p) const
{
    int a = pts[p], b = pts[1 - p];
    if (a >= 3 && b >= 3)
    {
        if (a == b) return "40";
        return a > b ? "AD" : "40";
    }
    static const char* t[4] = { "0", "15", "30", "40" };
    return t[std::min(a, 3)];
}

const char* Partido::playerName(int i) const { return NAMES[i]; }

void Partido::renderBoard()
{
    Canvas& c = board;
    glm::vec4 bg(0.015f, 0.04f, 0.12f, 1.0f), cyan(0.2f, 0.85f, 1.0f, 1.0f), white(1, 1, 1, 1), gold(1.0f, 0.82f, 0.15f, 1.0f);
    glm::vec4 dim(0.55f, 0.65f, 0.80f, 1.0f);
    c.fill(bg);
    c.rect(0, 0, 512, 6, cyan);
    c.rect(0, 250, 512, 6, cyan);
    c.rect(0, 0, 6, 256, cyan);
    c.rect(506, 0, 6, 256, cyan);
    c.text(18, 18, 3, "TENIS 3D", gold);
    const int colX[3] = { 236, 296, 356 };
    for (int i = 0; i < 3; ++i)
    {
        char lb[4]; std::snprintf(lb, 4, "S%d", i + 1);
        c.text(colX[i] - 2, 18, 3, lb, dim);
    }
    c.text(404, 18, 3, "PTS", dim);
    c.rect(14, 54, 484, 3, cyan);

    for (int p = 0; p < 2; ++p)
    {
        int y = 72 + p * 64;
        bool win = score.over && score.winner == p;
        glm::vec4 nameCol = win ? gold : white;
        if (score.server == p && !score.over) c.text(16, y, 4, "*", gold);
        c.text(44, y, 4, NAMES[p], nameCol);
        for (int k = 0; k < 3; ++k)
        {
            std::string v;
            glm::vec4 col = white;
            if (k < score.setNo) { char b[4]; std::snprintf(b, 4, "%d", score.sets[k][p]); v = b; col = (score.sets[k][p] > score.sets[k][1 - p]) ? gold : dim; }
            else if (k == score.setNo && !score.over) { char b[4]; std::snprintf(b, 4, "%d", score.games[p]); v = b; col = cyan; }
            else v = "-";
            c.text(colX[k], y, 4, v, col);
        }
        std::string pt = score.over ? "" : score.ptsText(p);
        c.text(404, y, 4, pt, gold);
        if (p == 0) c.rect(14, y + 50, 484, 2, glm::vec4(0.15f, 0.25f, 0.45f, 1.0f));
    }
    std::string foot;
    if (score.over) foot = std::string("GANA ") + NAMES[score.winner];
    else if (!banner.empty() && bannerTime > 0.0f) foot = banner.substr(0, 26);
    else foot = std::string("SAQUE: ") + NAMES[score.server];
    c.text(18, 214, 3, foot, score.over ? gold : white);
    boardDirty = false;
    ++boardVersion;
}

// ===========================================================================
//  Pistas de animación (clips)
// ===========================================================================
void Partido::Track::hold(float until, ClipType t)
{
    if (until <= curT + 1e-4f) return;
    Clip c; c.type = t; c.t0 = curT; c.t1 = until; c.p0 = c.p1 = curPos;
    clips.push_back(c);
    curT = until;
}

void Partido::Track::moveTo(const V2& p, float arrive)
{
    if (arrive <= curT + 0.02f) arrive = curT + 0.02f;
    Clip c; c.type = C_MOVE; c.t0 = curT; c.t1 = arrive; c.p0 = curPos; c.p1 = p;
    clips.push_back(c);
    curT = arrive;
    curPos = p;
}

void Partido::Track::action(ClipType t, float dur)
{
    Clip c; c.type = t; c.t0 = curT; c.t1 = curT + dur; c.p0 = c.p1 = curPos;
    clips.push_back(c);
    curT += dur;
}

// ===========================================================================
//  Inicialización
// ===========================================================================
V2 Partido::serveStart(int server, const Score& sc) const
{
    float zs = server == 0 ? 1.0f : -1.0f;
    bool deuce = ((sc.pts[0] + sc.pts[1]) % 2 == 0);
    return V2((deuce ? 1.5f : -1.5f) * zs, zs * (HL + PLAYER_BASELINE_OFFSET));
}

V2 Partido::recvStart(int server, const Score& sc) const
{
    V2 sp = serveStart(server, sc);
    float zs = server == 0 ? 1.0f : -1.0f;
    return V2(-U::sign(sp.x) * 1.0f, -zs * (HL + 1.7f));
}

void Partido::placeStart()
{
    V2 sp = serveStart(score.server, score), rp = recvStart(score.server, score);
    int s = score.server, r = 1 - s;
    actors_[s].pos = sp; actors_[r].pos = rp;
    for (int i = 0; i < 2; ++i) { actors_[i].track.begin(actors_[i].pos); actors_[i].yaw = actors_[i].faceYaw; }
}

void Partido::init(const Anchors& a)
{
    anchors_ = a;
    offServe_ = contactOffset(Anim::serveTrack(), Anim::SERVE_CONTACT);
    offFH_ = contactOffset(Anim::forehandTrack(), Anim::FH_CONTACT);

    // ---- vestimenta ----
    Outfit o0;
    o0.skin = V3(0.90f, 0.70f, 0.56f); o0.hair = V3(0.20f, 0.12f, 0.06f);
    o0.shirt = V3(0.88f, 0.12f, 0.12f); o0.trim = V3(1.0f); o0.shorts = V3(0.95f);
    o0.cap = V3(0.97f); o0.hasCap = true; o0.shoes = V3(0.95f); o0.socks = V3(0.95f);
    Outfit o1;
    o1.skin = V3(0.60f, 0.42f, 0.30f); o1.hair = V3(0.05f, 0.04f, 0.04f);
    o1.shirt = V3(0.10f, 0.36f, 0.86f); o1.trim = V3(1.0f, 0.85f, 0.10f); o1.shorts = V3(0.07f, 0.11f, 0.34f);
    o1.hasBand = true; o1.shoes = V3(0.95f, 0.95f, 0.30f); o1.socks = V3(0.95f);
    Outfit ou;
    ou.skin = V3(0.86f, 0.66f, 0.52f); ou.hair = V3(0.60f, 0.60f, 0.62f);
    ou.shirt = V3(0.95f); ou.trim = V3(0.10f, 0.14f, 0.40f); ou.shorts = V3(0.06f, 0.09f, 0.25f);
    ou.socks = V3(0.06f, 0.09f, 0.25f); ou.shoes = V3(0.08f); ou.longPants = true;
    Outfit ok0, ok1;
    ok0.skin = V3(0.80f, 0.60f, 0.46f); ok0.hair = V3(0.10f, 0.07f, 0.05f);
    ok0.shirt = V3(0.10f, 0.34f, 0.26f); ok0.trim = V3(1.0f); ok0.shorts = V3(0.10f, 0.12f, 0.12f);
    ok0.cap = V3(0.10f, 0.34f, 0.26f); ok0.hasCap = true; ok0.scale = 0.76f; ok0.shoes = V3(0.15f);
    ok1 = ok0; ok1.skin = V3(0.92f, 0.74f, 0.60f); ok1.scale = 0.72f;

    players[0].outfit = o0; players[1].outfit = o1; umpire.outfit = ou; kids[0].outfit = ok0; kids[1].outfit = ok1;
    umpire.hasRacket = false; kids[0].hasRacket = false; kids[1].hasRacket = false;
    umpire.autoGround = false;

    for (int i = 0; i < 2; ++i) { actors_[i].ch = &players[i]; actors_[i].kind = 0; actors_[i].faceYaw = yawOf(i); }
    for (int i = 0; i < 2; ++i) { actors_[2 + i].ch = &kids[i]; actors_[2 + i].kind = 1; }
    actors_[4].ch = &umpire; actors_[4].kind = 2; actors_[4].faceYaw = -90.0f;
    for (int i = 0; i < 5; ++i) actors_[i].seed = 1.7f * i;

    // recogepelotas en las esquinas del fondo (miran hacia el centro)
    for (int k = 0; k < 2; ++k)
    {
        V3 c = anchors_.kidCorner[k];
        actors_[2 + k].pos = V2(c.x, c.z);
        actors_[2 + k].faceYaw = c.z > 0 ? 180.0f : 0.0f;
        actors_[2 + k].yaw = actors_[2 + k].faceYaw;
        actors_[2 + k].track.begin(actors_[2 + k].pos);
    }
    umpire.pos = V3(anchors_.umpireSeat.x, anchors_.umpireSeat.y, 0.0f);
    umpire.yaw = -90.0f;
    actors_[4].yaw = -90.0f;

    reset();
}

void Partido::reset()
{
    score.reset();
    boardDirty = true;
    ballVisible = false;
    active_ = false; tPoint_ = tEnd_ = tPlayersEnd_ = 0.0f;
    segs_.clear(); evs_.clear();
    pendingWinner_ = -1;
    banner.clear(); bannerTime = 0.0f;
    placeStart();
    for (int k = 0; k < 2; ++k)
    {
        V3 c = anchors_.kidCorner[k];
        actors_[2 + k].pos = V2(c.x, c.z);
        actors_[2 + k].track.begin(actors_[2 + k].pos);
    }
    for (int i = 0; i < 5; ++i) { actors_[i].ci = 0; actors_[i].speed = 0.0f; }
    for (int i = 0; i < 5; ++i) evalActor(actors_[i], 0.0f);
    renderBoard();
}

bool Partido::canStart() const { return !active_ || tPoint_ >= tPlayersEnd_; }

glm::vec3 Partido::playerHead(int i) const { return actors_[i].head; }

// ===========================================================================
//  Planificación de un punto
// ===========================================================================
void Partido::planPoint()
{
    segs_.clear(); evs_.clear();
    events.clear();
    const int s = score.server, r = 1 - s;
    const float zs = (s == 0) ? 1.0f : -1.0f;
    const V2 sP = serveStart(s, score), rP = recvStart(s, score);
    const float xs = sP.x;

    // ---- fase de preparación: los jugadores caminan a su posición ----
    V2 cs = actors_[s].pos, cr = actors_[r].pos;
    float tSetup = std::max(0.9f, std::max(glm::distance(cs, sP), glm::distance(cr, rP)) / 2.3f);
    actors_[s].track.begin(cs); actors_[r].track.begin(cr);
    actors_[s].track.moveTo(sP, tSetup);
    actors_[r].track.moveTo(rP, tSetup);
    for (int k = 0; k < 2; ++k) actors_[2 + k].track.begin(actors_[2 + k].pos);

    float tFree[2]; V2 lastPos[2];
    tFree[s] = tFree[r] = tSetup;
    lastPos[s] = sP; lastPos[r] = rP;

    // ---- decisión del resultado ----
    enum Fate { F_WINNER, F_NET, F_OUT };
    int n; Fate fate;
    float u = rng_.next();
    if (u < 0.16f)      { n = 1; fate = F_WINNER; }                                   // ace
    else if (u < 0.22f) { n = 1; fate = rng_.chance(0.5f) ? F_NET : F_OUT; }          // doble falta
    else
    {
        n = rng_.irange(2, 7);
        float f = rng_.next();
        fate = f < 0.55f ? F_WINNER : (f < 0.78f ? F_NET : F_OUT);
    }

    struct Shot { int striker; float T; V3 hit; bool serve; };
    auto scheduleStroke = [&](int pl, const V2& stand, float Thit, bool serve)
    {
        Track& tr = actors_[pl].track;
        float tc = serve ? Anim::SERVE_CONTACT : Anim::FH_CONTACT;
        float tStart = Thit - tc;
        if (!serve)
        {
            float dist = glm::distance(lastPos[pl], stand);
            float moveDur = std::max(0.05f, dist / 4.8f);
            float moveStart = std::max(tFree[pl], tStart - moveDur);
            tr.hold(moveStart);
            tr.moveTo(stand, std::max(moveStart + 0.05f, tStart));
        }
        tr.hold(tStart);
        tr.action(serve ? C_SERVE : C_STROKE, serve ? Anim::SERVE_TOTAL : Anim::FH_TOTAL);
        tFree[pl] = tr.curT;
        lastPos[pl] = stand;
    };

    // ---- primer golpe: el saque ----
    Shot cur;
    cur.striker = s; cur.serve = true;
    cur.T = tSetup + Anim::SERVE_CONTACT;
    cur.hit = V3(sP.x, 0.0f, sP.y) + rotY(offServe_, yawOf(s));
    scheduleStroke(s, sP, cur.T, true);

    // lanzamiento de la pelota (arranca en la mano izquierda)
    {
        BallSeg b; b.t0 = tSetup + Anim::SERVE_RELEASE; b.t1 = cur.T; b.p0 = V3(0.0f); b.p1 = cur.hit; b.h = 0.95f;
        b.fromHand = true; b.captured = false;
        segs_.push_back(b);
    }
    tossSide_ = s;

    float tRest = 0.0f, tRes = 0.0f;
    V3 restPos(0.0f);
    int lastStriker = s;
    V3 lastLand(0.0f);
    float lastTB = 0.0f;
    int lastShotIdx = 0;

    for (int i = 0;; ++i)
    {
        bool last = (i == n - 1);
        lastShotIdx = i;
        int opp = 1 - cur.striker;
        float oz = (opp == 0) ? 1.0f : -1.0f;                // extremo del oponente
        float spd = cur.serve ? 21.0f : 14.0f;
        bool net = last && fate == F_NET;
        bool out = last && fate == F_OUT;
        bool win = last && fate == F_WINNER;
        lastStriker = cur.striker;
        { Ev e; e.t = cur.T; e.type = EV_HIT; e.done = false; evs_.push_back(e); }

        // ---------- punto de caída ----------
        V3 L;
        if (cur.serve)
        {
            float tx = -U::sign(xs) * rng_.range(1.0f, 3.4f);
            float tz = oz * rng_.range(3.8f, 5.9f);
            L = V3(tx, BALL_RADIUS, tz);
            if (out) L.z = oz * rng_.range(7.0f, 8.0f);                       // saque largo
            if (win) { L.x = -U::sign(xs) * rng_.range(3.0f, 3.9f); L.z = oz * rng_.range(5.2f, 6.2f); }
        }
        else
        {
            L = V3(rng_.range(-3.4f, 3.4f), BALL_RADIUS, oz * rng_.range(7.0f, 11.0f));
            if (win) { L.x = (rng_.chance(0.5f) ? 1.0f : -1.0f) * rng_.range(2.8f, 3.9f); L.z = oz * rng_.range(9.0f, 11.2f); }
            if (out) { L.z = oz * (HL + rng_.range(1.0f, 2.0f)); L.x = rng_.range(-3.0f, 3.0f); }
        }
        if (net)
        {
            float dz = std::fabs(cur.hit.z);
            V3 N((cur.hit.x + L.x) * 0.5f * 0.9f, rng_.range(0.50f, 0.78f), 0.0f);
            float tN = cur.T + std::max(0.45f, dz / spd);
            BallSeg a; a.t0 = cur.T; a.t1 = tN; a.p0 = cur.hit; a.p1 = N; a.h = 0.12f; a.fromHand = false; a.captured = true;
            segs_.push_back(a);
            V3 D(N.x * 0.95f, BALL_RADIUS, -oz * 0.45f);
            BallSeg b; b.t0 = tN; b.t1 = tN + 0.38f; b.p0 = N; b.p1 = D; b.h = 0.10f; b.fromHand = false; b.captured = true;
            segs_.push_back(b);
            V3 D2(D.x + 0.12f, BALL_RADIUS, D.z - oz * 0.35f);
            BallSeg c; c.t0 = tN + 0.38f; c.t1 = tN + 0.80f; c.p0 = D; c.p1 = D2; c.h = 0.07f; c.fromHand = false; c.captured = true;
            segs_.push_back(c);
            { Ev e; e.t = tN; e.type = EV_BOUNCE; e.done = false; evs_.push_back(e); }
            tRest = tN + 0.80f; restPos = D2; tRes = tN + 0.55f;
            lastLand = N; lastTB = tN;
            break;
        }

        float d1 = std::sqrt((L.x - cur.hit.x) * (L.x - cur.hit.x) + (L.z - cur.hit.z) * (L.z - cur.hit.z));
        float tB = cur.T + std::max(0.55f, d1 / spd);
        {
            BallSeg a; a.t0 = cur.T; a.t1 = tB; a.p0 = cur.hit; a.p1 = L;
            a.h = arcHeight(cur.hit, L, 1.30f, 0.30f); a.fromHand = false; a.captured = true;
            segs_.push_back(a);
            Ev e; e.t = tB; e.type = EV_BOUNCE; e.done = false; evs_.push_back(e);
        }
        lastLand = L; lastTB = tB;

        if (last)
        {
            // la pelota sigue hasta el muro de publicidad y se detiene
            V2 dir = glm::normalize(V2(L.x - cur.hit.x, L.z - cur.hit.z));
            float wz = oz * (FLOOR_HZ - 0.15f);
            float k = (wz - L.z) / (std::fabs(dir.y) > 0.2f ? dir.y : 0.2f * oz);
            float wx = U::clampf(L.x + dir.x * k, -FLOOR_HX + 0.4f, FLOOR_HX - 0.4f);
            V3 Wp(wx, 0.55f, wz);
            float tW = tB + std::max(0.45f, std::fabs(wz - L.z) / 8.0f);
            BallSeg b; b.t0 = tB; b.t1 = tW; b.p0 = L; b.p1 = Wp; b.h = 0.45f; b.fromHand = false; b.captured = true;
            segs_.push_back(b);
            V3 R(wx * 0.85f, BALL_RADIUS, oz * (FLOOR_HZ - 2.0f));
            BallSeg c; c.t0 = tW; c.t1 = tW + 0.55f; c.p0 = Wp; c.p1 = R; c.h = 0.28f; c.fromHand = false; c.captured = true;
            segs_.push_back(c);
            { Ev e; e.t = tW; e.type = EV_BOUNCE; e.done = false; evs_.push_back(e); }
            tRest = tW + 0.55f; restPos = R; tRes = tRest - 0.1f;

            if (win)
            {
                // el rival corre hacia la pelota y se estira, pero no llega
                int rc = opp;
                float yawR = yawOf(rc);
                V3 rightV = rotY(V3(-1.0f, 0.0f, 0.0f), yawR);
                V2 target = V2(L.x + dir.x * 1.0f - rightV.x * 0.6f, L.z + dir.y * 1.0f - rightV.z * 0.6f);
                target.x = U::clampf(target.x, -8.0f, 8.0f);
                float tArr = tB + 0.05f;
                float tStart = tArr - Anim::LUNGE_CONTACT;
                float avail = std::max(0.1f, tStart - tFree[rc]);
                V2 from = lastPos[rc];
                float dist = glm::distance(from, target);
                float maxD = avail * 6.0f;
                if (dist > maxD && dist > 0.001f) target = from + (target - from) * (maxD / dist);
                Track& tr = actors_[rc].track;
                tr.hold(tFree[rc]);
                tr.moveTo(target, std::max(tStart, tr.curT + 0.1f));
                tr.action(C_LUNGE, Anim::LUNGE_TOTAL);
                tFree[rc] = tr.curT;
                lastPos[rc] = target;
            }
            break;
        }

        // ---------- siguiente golpe del oponente ----------
        V2 dir = glm::normalize(V2(L.x - cur.hit.x, L.z - cur.hit.z));
        V2 hn(L.x + dir.x * 1.5f, L.z + dir.y * 1.5f);
        hn.x = U::clampf(hn.x, -7.0f, 7.0f);
        hn.y = oz * U::clampf(std::fabs(hn.y), 4.0f, HL + 1.6f);
        float yawN = yawOf(opp);
        V3 offR = rotY(offFH_, yawN);
        V2 stand(hn.x - offR.x, hn.y - offR.z);
        V3 hitN(stand.x + offR.x, offR.y, stand.y + offR.z);
        float d2 = std::sqrt((hitN.x - L.x) * (hitN.x - L.x) + (hitN.z - L.z) * (hitN.z - L.z));
        float tNext = tB + std::max(0.50f, d2 / (spd * 0.55f));
        float dist = glm::distance(lastPos[opp], stand);
        float tMin = tFree[opp] + dist / 4.6f + Anim::FH_CONTACT + 0.03f;
        if (tNext < tMin) tNext = tMin;
        {
            BallSeg b; b.t0 = tB; b.t1 = tNext; b.p0 = L; b.p1 = hitN; b.h = 0.55f; b.fromHand = false; b.captured = true;
            segs_.push_back(b);
        }
        scheduleStroke(opp, stand, tNext, false);
        Shot nx; nx.striker = opp; nx.T = tNext; nx.hit = hitN; nx.serve = false;
        cur = nx;
    }
    (void)lastLand; (void)lastTB; (void)lastShotIdx;

    // ---- resultado y celebración ----
    int ls = lastStriker;
    int winner = (fate == F_WINNER) ? ls : 1 - ls;
    std::string how;
    if (n == 1 && fate == F_WINNER) how = "ACE";
    else if (n == 1) how = "DOBLE FALTA";
    else if (fate == F_WINNER) how = "GANADOR";
    else if (fate == F_NET) how = "EN LA RED";
    else how = "AFUERA";
    pendingWinner_ = winner; pendingHow_ = how;
    { Ev e; e.t = tRes + 0.2f; e.type = EV_SCORE; e.done = false; evs_.push_back(e); }

    for (int p = 0; p < 2; ++p)
    {
        Track& tr = actors_[p].track;
        tr.hold(tRes);
        tr.action(p == winner ? C_CELEB : C_DEJECT, p == winner ? Anim::CELEBRATE_TOTAL : Anim::DEJECT_TOTAL);
    }
    tPlayersEnd_ = tRes + Anim::CELEBRATE_TOTAL + 0.2f;
    tEnd_ = tPlayersEnd_;

    // ---- el recogepelotas más cercano trae la pelota ----
    {
        int best = 0; float bd = 1e9f;
        for (int k = 0; k < 2; ++k)
        {
            float d = glm::distance(actors_[2 + k].pos, V2(restPos.x, restPos.z));
            if (d < bd) { bd = d; best = k; }
        }
        Actor& a = actors_[2 + best];
        V2 corner(anchors_.kidCorner[best].x, anchors_.kidCorner[best].z);
        a.track.begin(a.pos);
        float tGo = tRest + 0.4f;
        a.track.hold(tGo);
        float dist = glm::distance(a.pos, V2(restPos.x, restPos.z));
        a.track.moveTo(V2(restPos.x, restPos.z), tGo + dist / 5.4f);
        float tPick = a.track.curT;
        a.track.action(C_PICK, Anim::PICK_TOTAL);
        { Ev e; e.t = tPick + 0.5f; e.type = EV_HIDE_BALL; e.done = false; evs_.push_back(e); }
        float back = glm::distance(V2(restPos.x, restPos.z), corner);
        a.track.moveTo(corner, a.track.curT + back / 2.6f);
        a.track.hold(a.track.curT + 0.5f, C_CROUCH);
        tEnd_ = std::max(tEnd_, a.track.curT);
    }

    ballVisible = true;
    std::sort(evs_.begin(), evs_.end(), [](const Ev& x, const Ev& y) { return x.t < y.t; });
}

// ===========================================================================
//  Control del punto
// ===========================================================================
bool Partido::startPoint()
{
    if (!canStart() || score.over) return false;
    planPoint();
    tPoint_ = 0.0f;
    active_ = true;
    for (int i = 0; i < 5; ++i) actors_[i].ci = 0;
    idleTimer_ = 0.0f;
    setBanner("", 0.0f);
    boardDirty = true;
    return true;
}

void Partido::applyScoreEvent(int winner, const std::string& how)
{
    int r = score.addPoint(winner);
    scorePulse = 1.0f;
    cheer = 1.0f;
    boardDirty = true;
    std::string msg = std::string(how) + " - PUNTO " + NAMES[winner];
    if (r == 1) msg = std::string("JUEGO ") + NAMES[winner];
    else if (r == 2) msg = std::string("SET ") + NAMES[winner];
    else if (r == 3) msg = std::string("PARTIDO PARA ") + NAMES[winner] + "!";
    setBanner(msg, 4.0f);
    umpAnnounce_ = 2.4f;
    events.push_back(EV_APPLAUSE);
    if (score.over) autoPlay = false;
}

void Partido::quickPoint(int p)
{
    if (score.over) return;
    for (int i = 0; i < 2; ++i)
    {
        Track& tr = actors_[i].track;
        tr.begin(actors_[i].pos);
        tr.action(i == p ? C_CELEB : C_DEJECT, i == p ? Anim::CELEBRATE_TOTAL : Anim::DEJECT_TOTAL);
        actors_[i].ci = 0;
    }
    for (int k = 0; k < 2; ++k) { actors_[2 + k].track.begin(actors_[2 + k].pos); actors_[2 + k].ci = 0; }
    segs_.clear(); evs_.clear();
    ballVisible = false;
    pendingWinner_ = p; pendingHow_ = "PUNTO";
    Ev e; e.t = 0.05f; e.type = EV_SCORE; e.done = false; evs_.push_back(e);
    tPoint_ = 0.0f; tEnd_ = tPlayersEnd_ = Anim::CELEBRATE_TOTAL;
    active_ = true;
}

void Partido::playIntro()
{
    if (!canStart()) return;
    segs_.clear(); evs_.clear();
    ballVisible = false;
    // posiciones finales
    V2 sP = serveStart(score.server, score), rP = recvStart(score.server, score);
    V2 fin[2]; fin[score.server] = sP; fin[1 - score.server] = rP;
    float tMax = 0.0f;
    for (int i = 0; i < 2; ++i)
    {
        Track& tr = actors_[i].track;
        float off = i == 0 ? 0.8f : -0.8f;
        tr.begin(V2(off, TUNNEL_Z1 + 2.0f));
        actors_[i].pos = tr.curPos;
        float speed = (i == 0) ? 3.4f : 2.0f;
        auto go = [&](const V2& p) { tr.moveTo(p, tr.curT + glm::distance(tr.curPos, p) / speed); };
        if (i == 1) tr.hold(1.0f);
        go(V2(off, TUNNEL_Z0 + 0.5f));
        if (fin[i].y > 0.0f)
        {                                          // el jugador del norte rodea la red por el costado oeste
            go(V2(-7.4f, -15.0f)); go(V2(-7.4f, 9.0f)); go(fin[i]);
        }
        else go(fin[i]);
        actors_[i].ci = 0;
        tMax = std::max(tMax, tr.curT);
    }
    for (int k = 0; k < 2; ++k) { actors_[2 + k].track.begin(actors_[2 + k].pos); actors_[2 + k].ci = 0; }
    Ev e; e.t = 0.2f; e.type = EV_APPLAUSE; e.done = false; evs_.push_back(e);
    Ev e2; e2.t = tMax; e2.type = EV_APPLAUSE; e2.done = false; evs_.push_back(e2);
    tPoint_ = 0.0f; tPlayersEnd_ = tEnd_ = tMax + 0.3f; active_ = true;
    setBanner("BIENVENIDOS", 3.0f);
}

// ===========================================================================
//  Actualización por cuadro
// ===========================================================================
void Partido::evalActor(Actor& a, float dt)
{
    Character& ch = *a.ch;
    const float t = tPoint_;
    Pose pose;
    V2 pos = a.pos;
    float targetYaw = a.faceYaw;
    float speedInst = 0.0f;
    float headW = 1.0f;
    float turnRate = 540.0f;
    bool isMoving = false;

    const std::vector<Clip>& cl = a.track.clips;
    const Clip* clip = 0;
    if (!cl.empty())
    {
        if (a.ci >= cl.size()) a.ci = cl.size() - 1;
        while (a.ci + 1 < cl.size() && t >= cl[a.ci].t1) ++a.ci;
        while (a.ci > 0 && t < cl[a.ci].t0) --a.ci;
        clip = &cl[a.ci];
    }

    // pose base en reposo
    Pose base = a.kind == 0 ? Anim::ready() : (a.kind == 1 ? Anim::kidCrouch() : Anim::umpireSeated());
    pose = base;
    ClipType type = C_IDLE;
    if (clip)
    {
        type = clip->type;
        if (t > clip->t1 && a.ci + 1 >= cl.size()) type = (a.kind == 1 ? C_CROUCH : C_IDLE);
        float len = std::max(0.001f, clip->t1 - clip->t0);
        float lt = t - clip->t0;
        if (type == C_MOVE && t <= clip->t1)
        {
            float uu = U::clamp01(lt / len);
            pos = clip->p0 + (clip->p1 - clip->p0) * uu;
            speedInst = glm::distance(clip->p0, clip->p1) / len;
            V2 d = clip->p1 - clip->p0;
            if (glm::length(d) > 1.2f && (clip->t1 - t) > 0.45f) targetYaw = U::deg(std::atan2(d.x, d.y));
            isMoving = true;
        }
        else
        {
            pos = (t > clip->t1) ? clip->p1 : clip->p0;
            if (type == C_MOVE) pos = clip->p1;
        }
        switch (type)
        {
        case C_SERVE:  pose = Anim::evalTrack(Anim::serveTrack(), lt); turnRate = 1500.0f; break;
        case C_STROKE: pose = Anim::evalTrack(Anim::forehandTrack(), lt); turnRate = 1500.0f; break;
        case C_LUNGE:  pose = Anim::evalTrack(Anim::lungeTrack(), lt); turnRate = 1500.0f; headW = 0.6f; break;
        case C_CELEB:  pose = Anim::evalTrack(Anim::celebrateTrack(), lt); headW = 0.0f; break;
        case C_DEJECT: pose = Anim::evalTrack(Anim::dejectTrack(), lt); headW = 0.0f; break;
        case C_PICK:   pose = Anim::evalTrack(Anim::pickTrack(), lt); headW = 0.4f; break;
        default: break;
        }
        if (type == C_MOVE && a.kind == 1) pose = Anim::kidReady();
    }
    if (type == C_IDLE && a.kind == 1) pose = Anim::kidCrouch();
    if (type == C_MOVE && a.kind == 1) pose = Anim::kidReady();

    // locomoción (solo cuando se desplaza)
    a.speed += (speedInst - a.speed) * U::damp(12.0f, dt);
    a.phase += a.speed * dt * (2.0f * U::PI / 2.3f);
    if (a.kind != 2 && (type == C_MOVE || (a.speed > 0.4f && (type == C_IDLE || type == C_CROUCH))))
        Anim::locomotion(pose, a.phase, a.speed, a.kind == 0);

    // respiración / balanceo en reposo
    if (type == C_IDLE || type == C_CROUCH)
    {
        float b = std::sin(time_ * 2.1f + a.seed) * (a.kind == 2 ? 0.8f : 2.2f);
        pose[J_RKNEE] += b; pose[J_LKNEE] += b; pose[J_TORSO_PITCH] += b * 0.3f;
    }
    // umpire: anuncia el marcador levantando el micrófono
    if (a.kind == 2 && umpAnnounce_ > 0.0f)
    {
        float k = U::smooth(std::min(umpAnnounce_, 0.4f) / 0.4f) * U::smooth((2.4f - umpAnnounce_) / 0.3f + 0.0f);
        pose = Pose::mix(pose, Anim::umpireAnnounce(), U::clamp01(k));
    }

    Anim::flatFeet(pose);

    // seguimiento de la pelota con la cabeza
    if (ballVisible && headW > 0.0f)
    {
        V3 d = ballPos - a.head;
        float ang = U::deg(std::atan2(d.x, d.z));
        float rel = U::wrap180(ang - ch.yaw);
        float lim = a.kind == 2 ? 75.0f : 65.0f;
        rel = U::clampf(rel, -lim, lim);
        float hz = std::sqrt(d.x * d.x + d.z * d.z);
        float pit = U::clampf(-U::deg(std::atan2(d.y, std::max(hz, 0.3f))), -35.0f, 30.0f);
        a.headYaw += (rel - a.headYaw) * U::damp(8.0f, dt);
        a.headPitch += (pit - a.headPitch) * U::damp(8.0f, dt);
    }
    else if (a.kind == 2 || a.kind == 1)
    {
        // sin pelota: miran a la cancha
        a.headYaw += (0.0f - a.headYaw) * U::damp(4.0f, dt);
        a.headPitch += (4.0f - a.headPitch) * U::damp(4.0f, dt);
    }
    else
    {
        a.headYaw += (0.0f - a.headYaw) * U::damp(4.0f, dt);
        a.headPitch += (0.0f - a.headPitch) * U::damp(4.0f, dt);
    }
    pose[J_HEAD_YAW] += a.headYaw * headW;
    pose[J_HEAD_PITCH] += a.headPitch * headW;

    // giro del cuerpo
    if (a.kind == 2) a.yaw = -90.0f;
    else a.yaw = U::approachAngle(a.yaw, targetYaw, turnRate * dt + (dt == 0.0f ? 720.0f : 0.0f));
    a.pos = pos;

    ch.yaw = a.yaw;
    ch.pose = pose;
    if (a.kind == 2) ch.pos = V3(anchors_.umpireSeat.x, anchors_.umpireSeat.y, 0.0f);
    else ch.pos = V3(pos.x, 0.0f, pos.y);
    a.head = ch.headPos(ch.skeleton());
}

void Partido::update(float dt)
{
    time_ += dt;
    if (active_) tPoint_ += dt;
    if (umpAnnounce_ > 0.0f) umpAnnounce_ = std::max(0.0f, umpAnnounce_ - dt);

    for (int i = 0; i < 5; ++i) evalActor(actors_[i], dt);

    // ---- pelota ----
    if (ballVisible)
    {
        const float t = tPoint_;
        V3 p = ballPos;
        if (!segs_.empty())
        {
            Character& sv = players[tossSide_];
            if (t < segs_[0].t0)
            {
                V3 hp = sv.handPos(sv.skeleton(), false);
                p = hp + V3(0.0f, 0.03f, 0.0f);
            }
            else
            {
                size_t k = 0;
                while (k + 1 < segs_.size() && t >= segs_[k].t1) ++k;
                BallSeg& sg = segs_[k];
                if (sg.fromHand && !sg.captured)
                {
                    sg.p0 = sv.handPos(sv.skeleton(), false) + V3(0.0f, 0.03f, 0.0f);
                    sg.captured = true;
                }
                float uu = U::clamp01((t - sg.t0) / std::max(0.001f, sg.t1 - sg.t0));
                p = glm::mix(sg.p0, sg.p1, uu);
                p.y = sg.p0.y * (1.0f - uu) + sg.p1.y * uu + 4.0f * sg.h * uu * (1.0f - uu);
            }
        }
        V3 vel = (dt > 0.0f) ? (p - ballPos) / dt : V3(0.0f);
        float sp = glm::length(vel);
        if (sp > 0.3f && sp < 80.0f && dt > 0.0f)
        {
            V3 axis = glm::cross(V3(0, 1, 0), vel / sp);
            if (glm::length(axis) > 1e-4f)
                ballRot_ = glm::rotate(M4(1.0f), sp * dt / BALL_RADIUS * 0.35f, glm::normalize(axis)) * ballRot_;
        }
        ballPos = p;
    }

    // ---- eventos de la línea de tiempo ----
    if (active_)
    {
        for (size_t i = 0; i < evs_.size(); ++i)
        {
            Ev& e = evs_[i];
            if (e.done || tPoint_ < e.t) continue;
            e.done = true;
            switch (e.type)
            {
            case EV_HIT: case EV_BOUNCE: case EV_APPLAUSE: events.push_back(e.type); break;
            case EV_SCORE: applyScoreEvent(pendingWinner_, pendingHow_); break;
            case EV_HIDE_BALL: ballVisible = false; break;
            default: break;
            }
        }
        if (tPoint_ >= tEnd_) { active_ = false; idleTimer_ = 0.0f; }
    }
    else
    {
        idleTimer_ += dt;
        if (autoPlay && !score.over && idleTimer_ > 1.4f) startPoint();
    }

    scorePulse = std::max(0.0f, scorePulse - dt * 0.9f);
    cheer = std::max(0.0f, cheer - dt * 0.28f);
    if (bannerTime > 0.0f) { bannerTime -= dt; if (bannerTime <= 0.0f) { banner.clear(); boardDirty = true; } }
    catT_ += dt;
    if (boardDirty) renderBoard();
}

// ===========================================================================
//  Dibujo
// ===========================================================================
void Partido::draw(Renderer& r, const Assets& A)
{
    players[0].draw(r, &A.racket);
    players[1].draw(r, &A.racket);
    umpire.draw(r, 0);
    kids[0].draw(r, 0);
    kids[1].draw(r, 0);

    // pelota (se dibuja un poco más grande para que se aprecie desde las gradas)
    if (ballVisible)
    {
        const float k = 1.7f;
        M4 m = T(ballPos.x, ballPos.y, ballPos.z) * ballRot_ * S(k) * T(0.0f, -BALL_RADIUS, 0.0f);
        r.drawModel(A.ball, m);
    }

    // ---- mascotas: el gato patrulla junto al muro oeste; el perro vigila el túnel ----
    {
        const float len = 26.0f, speed = 0.45f;
        float u = std::fmod(catT_ * speed, 2.0f * len);
        bool fwd = u < len;
        float z = fwd ? -13.0f + u : 13.0f - (u - len);
        float yaw = (fwd ? 0.0f : 180.0f) + CAT_FACING_OFFSET;
        float sc = 0.42f / std::max(0.01f, A.cat.size().y);
        float sway = std::sin(catT_ * 6.0f) * 2.5f;
        M4 m = T(-9.9f, 0.0f, z) * RY(yaw) * glm::rotate(M4(1.0f), U::rad(sway), V3(0, 0, 1)) * S(sc) * T(0.0f, -A.cat.bmin.y, 0.0f);
        r.drawModel(A.cat, m);

        float ds = 0.60f / std::max(0.01f, A.dog.size().y);
        float breathe = 1.0f + 0.015f * std::sin(time_ * 2.4f);
        M4 d = T(3.9f, 0.0f, -17.6f) * RY(180.0f + DOG_FACING_OFFSET) * glm::scale(M4(1.0f), V3(ds, ds * breathe, ds)) * T(0.0f, -A.dog.bmin.y, 0.0f);
        r.drawModel(A.dog, d);
    }
}
