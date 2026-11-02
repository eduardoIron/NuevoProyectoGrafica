// ============================================================================
//  Partido.h  -  Lógica del partido: marcador de tenis + coreografía de cada punto
//
//  Cada punto se "planea" como una LÍNEA DE TIEMPO (keyframes de alto nivel):
//    - clips por personaje: mover, saque, derecha, estirada, festejo, desánimo...
//    - segmentos de trayectoria de la pelota (parábolas con rebote)
//    - eventos (golpe, rebote, aplausos, cambio de marcador)
//  Los puntos de contacto se calculan con el esqueleto del jugador, de modo que
//  la pelota siempre coincide con la raqueta.
// ============================================================================
#pragma once

#include "Animacion.h"
#include "Estadio.h"
#include "Font.h"
#include "Model.h"
#include <string>
#include <vector>

enum MatchEventType { EV_HIT = 0, EV_BOUNCE, EV_APPLAUSE, EV_SCORE, EV_HIDE_BALL, EV_NONE };

struct Score
{
    int pts[2];
    int games[2];
    int sets[3][2];
    int setNo;
    int server;
    bool over;
    int winner;

    Score() { reset(); }
    void reset();
    // suma un punto; devuelve 0 = nada, 1 = ganó el juego, 2 = ganó el set, 3 = ganó el partido
    int addPoint(int p);
    std::string ptsText(int p) const;
};

class Partido
{
public:
    Character players[2], umpire, kids[2];
    Score score;
    bool ballVisible = false;
    glm::vec3 ballPos = glm::vec3(0.0f);
    Canvas board;
    bool boardDirty = true;
    int boardVersion = 0;                  // se incrementa cada vez que se repinta el marcador
    float scorePulse = 0.0f;
    float cheer = 0.0f;
    std::string banner;
    float bannerTime = 0.0f;
    bool autoPlay = false;
    std::vector<int> events;               // eventos de audio/HUD pendientes (MatchEventType)

    Partido() : board(512, 256) {}
    void init(const Anchors& a);
    void reset();
    void update(float dt);
    bool startPoint();                     // inicia un punto completo (saque + intercambio)
    void quickPoint(int p);                // da un punto directo al jugador p con festejo
    void playIntro();                      // entrada de los jugadores desde el túnel
    bool canStart() const;
    void draw(Renderer& r, const Assets& A);

    glm::vec3 playerHead(int i) const;
    bool busy() const { return !canStart(); }
    void setBannerPublic(const std::string& s, float secs) { banner = s; bannerTime = secs; }
    const char* playerName(int i) const;

private:
    enum ClipType { C_IDLE, C_MOVE, C_SERVE, C_STROKE, C_LUNGE, C_CELEB, C_DEJECT, C_PICK, C_CROUCH };
    struct Clip { ClipType type; float t0, t1; glm::vec2 p0, p1; };
    struct Track
    {
        std::vector<Clip> clips;
        glm::vec2 curPos;
        float curT = 0.0f;
        void begin(const glm::vec2& p) { clips.clear(); curPos = p; curT = 0.0f; }
        void hold(float until, ClipType t = C_IDLE);
        void moveTo(const glm::vec2& p, float arrive);
        void action(ClipType t, float dur);
    };
    struct Actor
    {
        Character* ch = 0;
        int kind = 0;                       // 0 jugador, 1 recogepelotas, 2 juez
        Track track;
        size_t ci = 0;
        float phase = 0.0f, speed = 0.0f, yaw = 0.0f, faceYaw = 0.0f;
        float headYaw = 0.0f, headPitch = 0.0f;
        glm::vec3 head = glm::vec3(0.0f);
        glm::vec2 pos = glm::vec2(0.0f);
        float seed = 0.0f;
    };
    struct BallSeg { float t0, t1; glm::vec3 p0, p1; float h; bool fromHand; bool captured; };
    struct Ev { float t; int type; bool done; };

    Anchors anchors_;
    Actor actors_[5];                       // 0,1 jugadores; 2,3 recogepelotas; 4 juez
    std::vector<BallSeg> segs_;
    std::vector<Ev> evs_;
    float tPoint_ = 0.0f, tEnd_ = 0.0f, tPlayersEnd_ = 0.0f, time_ = 0.0f, idleTimer_ = 0.0f;
    bool active_ = false;
    int tossSide_ = 0;
    glm::vec3 ballRotAxis_ = glm::vec3(1, 0, 0);
    glm::mat4 ballRot_ = glm::mat4(1.0f);
    glm::vec3 lastBall_ = glm::vec3(0.0f);
    float umpAnnounce_ = 0.0f;
    glm::vec3 offServe_, offFH_;
    U::Rng rng_;
    // mascotas
    float catT_ = 0.0f;

    void planPoint();
    void renderBoard();
    void evalActor(Actor& a, float dt);
    void setBanner(const std::string& s, float secs) { banner = s; bannerTime = secs; }
    void applyScoreEvent(int winner, const std::string& how);
    int pendingWinner_ = -1;
    std::string pendingHow_;
    void placeStart();
    glm::vec2 serveStart(int server, const Score& sc) const;
    glm::vec2 recvStart(int server, const Score& sc) const;
};
