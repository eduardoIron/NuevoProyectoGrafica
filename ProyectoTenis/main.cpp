// ============================================================================
//  Proyecto Final - Escenario Deportivo 3D Interactivo: TENIS
//  Computación Gráfica e Interacción Humano-Computadora
//  OpenGL 3.3 (core) + GLEW + GLFW + GLM + Assimp + SOIL2
//
//  main.cpp: ventana, entrada, bucle principal y orquestación de módulos
// ============================================================================
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#if defined(_WIN32)
#include <direct.h>
#define CHDIR _chdir
#else
#include <unistd.h>
#define CHDIR chdir
#endif

#include "Utils.h"
#include "Renderer.h"
#include "Texturas.h"
#include "Model.h"
#include "Estadio.h"
#include "Partido.h"
#include "Camara.h"
#include "Font.h"
#include "Audio.h"

// ---------------------------------------------------------------------------
//  Estado global de la aplicación
// ---------------------------------------------------------------------------
struct App
{
    GLFWwindow* window = 0;
    int fbW = 1280, fbH = 720;
    bool mouseCaptured = true, firstMouse = true;
    double lastX = 0, lastY = 0;
    float mdx = 0, mdy = 0, scroll = 0;
    bool showHelp = true, paused = false, wire = false;

    float hour = 15.5f, hourTarget = 15.5f;
    bool floodOn = false;
    float floodLevel = 0.0f;
    int surface = SURF_CLAY;
    bool wave = false;
    float waveOn = 0.0f, waveTime = 0.0f;
    float time = 0.0f;
    int shotCounter = 0;
};
static App g;

static CameraRig g_cam;
static Partido g_match;
static Estadio g_stadium;

static float groundHeightThunk(void* ctx, float x, float z) { return static_cast<Estadio*>(ctx)->groundHeight(x, z); }

static const char* surfaceName(int s) { static const char* n[3] = { "ARCILLA", "PASTO", "CEMENTO" }; return n[s]; }

// ---------------------------------------------------------------------------
//  Utilidades
// ---------------------------------------------------------------------------
static void fixWorkingDirectory(const char* argv0)
{
    if (U::fileExists("Shader/escena.vs")) return;
    std::string exe = argv0 ? argv0 : "";
    size_t slash = exe.find_last_of("/\\");
    std::string dir = (slash == std::string::npos) ? "." : exe.substr(0, slash);
    const char* rel[] = { "", "/..", "/../ProyectoTenis", "/../../ProyectoTenis", "/ProyectoTenis" };
    for (int i = 0; i < 5; ++i)
    {
        std::string c = dir + rel[i];
        if (U::fileExists(c + "/Shader/escena.vs")) { CHDIR(c.c_str()); return; }
    }
}

static void saveScreenshot(const std::string& path)
{
    std::vector<unsigned char> px(static_cast<size_t>(g.fbW) * g.fbH * 3), fl(px.size());
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, g.fbW, g.fbH, GL_RGB, GL_UNSIGNED_BYTE, &px[0]);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    for (int y = 0; y < g.fbH; ++y)
        std::memcpy(&fl[static_cast<size_t>(y) * g.fbW * 3], &px[static_cast<size_t>(g.fbH - 1 - y) * g.fbW * 3], static_cast<size_t>(g.fbW) * 3);
    if (U::saveBMP(path, &fl[0], g.fbW, g.fbH)) std::cout << "Captura guardada: " << path << std::endl;
}

static CamScene buildCamScene()
{
    CamScene s;
    s.ball = g_match.ballVisible ? g_match.ballPos : glm::vec3(0.0f, 1.0f, 0.0f);
    s.player[0] = g_match.players[0].pos;
    s.player[1] = g_match.players[1].pos;
    s.playerHead = g_match.playerHead(0);
    s.playerFwd = g_match.players[0].forward();
    s.umpireEye = g_stadium.anchors.umpireEye;
    s.tvPos[0] = g_stadium.anchors.tvCam[0];
    s.tvPos[1] = g_stadium.anchors.tvCam[1];
    s.groundHeight = groundHeightThunk;
    s.ctx = &g_stadium;
    return s;
}

// ---------------------------------------------------------------------------
//  Callbacks de GLFW
// ---------------------------------------------------------------------------
static void framebufferCb(GLFWwindow*, int w, int h) { g.fbW = w > 1 ? w : 1; g.fbH = h > 1 ? h : 1; }

static void mouseCb(GLFWwindow*, double x, double y)
{
    if (g.firstMouse) { g.lastX = x; g.lastY = y; g.firstMouse = false; }
    if (g.mouseCaptured) { g.mdx += static_cast<float>(x - g.lastX); g.mdy += static_cast<float>(y - g.lastY); }
    g.lastX = x; g.lastY = y;
}

static void scrollCb(GLFWwindow*, double, double dy) { g.scroll += static_cast<float>(dy); }

static void setMode(CamMode m) { g_cam.setMode(m, buildCamScene()); }

static void keyCb(GLFWwindow* w, int key, int, int action, int)
{
    if (action != GLFW_PRESS) return;
    switch (key)
    {
    case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(w, GL_TRUE); break;
    case GLFW_KEY_1: setMode(CAM_LIBRE); break;
    case GLFW_KEY_2: setMode(CAM_ESPECTADOR); break;
    case GLFW_KEY_3: setMode(CAM_JUEZ); break;
    case GLFW_KEY_4: setMode(CAM_TV); break;
    case GLFW_KEY_5: setMode(CAM_TERCERA); break;
    case GLFW_KEY_6: setMode(CAM_CENITAL); break;
    case GLFW_KEY_7: setMode(CAM_JUGADOR); break;
    case GLFW_KEY_C: g_cam.cycleOption(buildCamScene()); break;
    case GLFW_KEY_L: g.floodOn = !g.floodOn; break;
    case GLFW_KEY_N:
        if (g.hourTarget < 18.0f || g.hourTarget > 24.0f) { g.hourTarget = 22.0f; g.floodOn = true; }
        else { g.hourTarget = 15.5f; g.floodOn = false; }
        break;
    case GLFW_KEY_COMMA: g.hourTarget = std::fmod(g.hourTarget + 23.5f, 24.0f); break;
    case GLFW_KEY_PERIOD: g.hourTarget = std::fmod(g.hourTarget + 0.5f, 24.0f); break;
    case GLFW_KEY_T: g.surface = (g.surface + 1) % SURF_COUNT; break;
    case GLFW_KEY_E: if (!g_match.startPoint()) g_match.setBannerPublic("ESPERA A QUE TERMINE EL PUNTO", 1.5f); break;
    case GLFW_KEY_Q: g_match.autoPlay = !g_match.autoPlay; g_match.setBannerPublic(g_match.autoPlay ? "MODO AUTOMATICO: ACTIVADO" : "MODO AUTOMATICO: APAGADO", 2.0f); break;
    case GLFW_KEY_J: g_match.quickPoint(0); break;
    case GLFW_KEY_K: g_match.quickPoint(1); break;
    case GLFW_KEY_I: g_match.playIntro(); break;
    case GLFW_KEY_R: g_match.reset(); break;
    case GLFW_KEY_V: Audio::play(SND_WHISTLE); break;
    case GLFW_KEY_B: g.wave = !g.wave; break;
    case GLFW_KEY_O: g_match.cheer = 1.0f; Audio::play(SND_APPLAUSE); break;
    case GLFW_KEY_H: g.showHelp = !g.showHelp; break;
    case GLFW_KEY_P: g.paused = !g.paused; break;
    case GLFW_KEY_X:
        g.wire = !g.wire;
        glPolygonMode(GL_FRONT_AND_BACK, g.wire ? GL_LINE : GL_FILL);
        break;
    case GLFW_KEY_TAB:
        g.mouseCaptured = !g.mouseCaptured;
        glfwSetInputMode(w, GLFW_CURSOR, g.mouseCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        g.firstMouse = true;
        break;
    case GLFW_KEY_F12:
    {
        char name[64];
        std::snprintf(name, sizeof(name), "captura_%03d.bmp", ++g.shotCounter);
        saveScreenshot(name);
        break;
    }
    default: break;
    }
}

// ---------------------------------------------------------------------------
//  HUD
// ---------------------------------------------------------------------------
static void drawHud(HudText& hud, float fps)
{
    hud.begin(g.fbW, g.fbH);
    float sc = g.fbW >= 1100 ? 2.0f : 1.5f;
    glm::vec4 white(1, 1, 1, 1), gold(1.0f, 0.85f, 0.2f, 1.0f), cyan(0.4f, 0.9f, 1.0f, 1.0f);

    char line[256];
    int hh = static_cast<int>(g.hour), mm = static_cast<int>((g.hour - hh) * 60.0f);
    std::snprintf(line, sizeof(line), "CAMARA: %s   FPS: %d", g_cam.modeName(), static_cast<int>(fps + 0.5f));
    hud.rect(8, 8, hud.width(line, sc) + 16, 8 * sc + 10, glm::vec4(0, 0, 0, 0.45f));
    hud.text(16, 13, sc, line, white);
    std::snprintf(line, sizeof(line), "%02d:%02d   REFLECTORES: %s   SUPERFICIE: %s%s", hh, mm, g.floodOn ? "ENCENDIDOS" : "APAGADOS",
                 surfaceName(g.surface), g_match.autoPlay ? "   [AUTO]" : "");
    hud.rect(8, 12 + 8 * sc + 8, hud.width(line, sc) + 16, 8 * sc + 10, glm::vec4(0, 0, 0, 0.45f));
    hud.text(16, 12 + 8 * sc + 13, sc, line, cyan);

    if (!g_match.banner.empty() && g_match.bannerTime > 0.0f)
    {
        float bs = sc * 1.8f;
        float bw = hud.width(g_match.banner, bs);
        float x = (g.fbW - bw) * 0.5f, y = g.fbH * 0.14f;
        hud.rect(x - 16, y - 10, bw + 32, 8 * bs + 20, glm::vec4(0, 0, 0, 0.55f));
        hud.text(x, y, bs, g_match.banner, gold);
    }

    if (g.showHelp)
    {
        const char* L[] = {
            "MOVER: W A S D   MIRAR: MOUSE   SUBIR/BAJAR: ESPACIO/CTRL   RAPIDO: SHIFT   ZOOM: RUEDA",
            "CAMARAS: 1 LIBRE  2 ESPECTADOR  3 JUEZ  4 TV  5 TERCERA PERSONA  6 CENITAL  7 JUGADOR   C: CAMBIAR VISTA",
            "E: JUGAR PUNTO   Q: AUTOMATICO   J / K: PUNTO A JUGADOR 1 / 2   I: ENTRADA DE JUGADORES   R: REINICIAR",
            "L: REFLECTORES   N: DIA/NOCHE   , / . : HORA   T: SUPERFICIE   B: OLA   O: OVACION   V: SILBATO",
            "H: AYUDA   P: PAUSA   X: ALAMBRE   TAB: LIBERAR MOUSE   F12: CAPTURA   ESC: SALIR" };
        float lh = 8 * sc + 4;
        float h = 5 * lh + 16;
        float w = 0;
        for (int i = 0; i < 5; ++i) w = std::max(w, hud.width(L[i], sc));
        float y0 = g.fbH - h - 8;
        hud.rect(8, y0, w + 24, h, glm::vec4(0, 0, 0, 0.55f));
        for (int i = 0; i < 5; ++i) hud.text(20, y0 + 8 + i * lh, sc, L[i], i == 0 ? gold : white);
    }
    hud.end();
}

// ---------------------------------------------------------------------------
//  Programa principal
// ---------------------------------------------------------------------------
int main(int argc, char** argv)
{
    fixWorkingDirectory(argv[0]);

    // --- modo captura (para pruebas/documentación):  --shot archivo.bmp [opciones]
    struct Dev
    {
        bool on = false; std::string file;
        int cam = 0; float hour = -1; int flood = -1; int surf = -1; float sim = 0; bool point = false, intro = false, wave = false, autoplay = false;
        bool hasPos = false; glm::vec3 pos, look; int w = 1280, h = 720; int tv = 0; int quick = -1;
    } dev;
    for (int i = 1; i < argc; ++i)
    {
        std::string a = argv[i];
        if (a == "--shot" && i + 1 < argc) { dev.on = true; dev.file = argv[++i]; }
        else if (a == "--cam" && i + 1 < argc) dev.cam = std::atoi(argv[++i]);
        else if (a == "--hour" && i + 1 < argc) dev.hour = static_cast<float>(std::atof(argv[++i]));
        else if (a == "--flood" && i + 1 < argc) dev.flood = std::atoi(argv[++i]);
        else if (a == "--surf" && i + 1 < argc) dev.surf = std::atoi(argv[++i]);
        else if (a == "--sim" && i + 1 < argc) dev.sim = static_cast<float>(std::atof(argv[++i]));
        else if (a == "--point") dev.point = true;
        else if (a == "--intro") dev.intro = true;
        else if (a == "--wave") dev.wave = true;
        else if (a == "--auto") dev.autoplay = true;
        else if (a == "--tv" && i + 1 < argc) dev.tv = std::atoi(argv[++i]);
        else if (a == "--quick" && i + 1 < argc) dev.quick = std::atoi(argv[++i]);
        else if (a == "--size" && i + 2 < argc) { dev.w = std::atoi(argv[++i]); dev.h = std::atoi(argv[++i]); }
        else if (a == "--pos" && i + 6 < argc)
        {
            dev.hasPos = true;
            dev.pos = glm::vec3(static_cast<float>(std::atof(argv[i + 1])), static_cast<float>(std::atof(argv[i + 2])), static_cast<float>(std::atof(argv[i + 3])));
            dev.look = glm::vec3(static_cast<float>(std::atof(argv[i + 4])), static_cast<float>(std::atof(argv[i + 5])), static_cast<float>(std::atof(argv[i + 6])));
            i += 6;
        }
    }

    if (!glfwInit()) { std::cout << "No se pudo iniciar GLFW" << std::endl; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, dev.on ? 0 : 4);

    g.fbW = dev.on ? dev.w : 1280; g.fbH = dev.on ? dev.h : 720;
    g.window = glfwCreateWindow(g.fbW, g.fbH, "Proyecto Final - Escenario Deportivo 3D: TENIS", NULL, NULL);
    if (!g.window) { std::cout << "No se pudo crear la ventana (se requiere OpenGL 3.3)" << std::endl; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(g.window);
    glfwSwapInterval(1);
    glfwGetFramebufferSize(g.window, &g.fbW, &g.fbH);

    glfwSetFramebufferSizeCallback(g.window, framebufferCb);
    glfwSetKeyCallback(g.window, keyCb);
    glfwSetCursorPosCallback(g.window, mouseCb);
    glfwSetScrollCallback(g.window, scrollCb);
    if (!dev.on) glfwSetInputMode(g.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    else g.mouseCaptured = false;

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { std::cout << "No se pudo iniciar GLEW" << std::endl; return -1; }
    glGetError();                                   // GLEW puede dejar un error pendiente en core profile
    std::cout << "OpenGL " << glGetString(GL_VERSION) << " | " << glGetString(GL_RENDERER) << std::endl;
    if (!dev.on) glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);

    // --- carga de recursos ---
    Renderer renderer;
    if (!renderer.init()) { std::cout << "Error al compilar los shaders (revisa la carpeta Shader y el directorio de trabajo)." << std::endl; return -1; }
    HudText hud;
    if (!hud.init()) { std::cout << "Error al iniciar el HUD." << std::endl; return -1; }
    Textures textures;
    textures.generateAll();
    Assets assets;
    if (!assets.load()) std::cout << "AVISO: no se pudieron cargar todos los modelos (carpeta Models)." << std::endl;
    g_stadium.build(textures);
    g_match.init(g_stadium.anchors);
    Audio::init();

    CamScene cs0 = buildCamScene();
    g_cam.init(cs0);
    g_match.update(0.0f);
    g_match.setBannerPublic("PRESIONA E PARA JUGAR UN PUNTO  -  H: AYUDA", 6.0f);

    // --- configuración inicial del modo captura ---
    if (dev.on)
    {
        if (dev.hour >= 0) { g.hour = g.hourTarget = dev.hour; }
        if (dev.flood >= 0) { g.floodOn = dev.flood != 0; g.floodLevel = g.floodOn ? 1.0f : 0.0f; }
        if (dev.surf >= 0) g.surface = dev.surf;
        if (dev.wave) { g.wave = true; g.waveOn = 1.0f; g.waveTime = 1.3f; }
        if (dev.intro) g_match.playIntro();
        if (dev.point) g_match.startPoint();
        if (dev.quick >= 0) g_match.quickPoint(dev.quick);
        if (dev.autoplay) g_match.autoPlay = true;
        for (float t = 0; t < dev.sim; t += 1.0f / 30.0f) g_match.update(1.0f / 30.0f);
        if (dev.cam >= 1) setMode(static_cast<CamMode>(dev.cam - 1));
        if (dev.tv) g_cam.cycleOption(buildCamScene());
        { CamInput none; for (int i = 0; i < 90; ++i) g_cam.update(1.0f / 30.0f, none, buildCamScene()); }   // deja converger la suavización
        g.showHelp = false;
    }

    double prev = glfwGetTime();
    float fpsAcc = 0, fps = 60; int fpsN = 0;
    int shotFrames = 0;

    while (!glfwWindowShouldClose(g.window))
    {
        double now = glfwGetTime();
        float dt = static_cast<float>(now - prev);
        prev = now;
        if (dt > 0.05f) dt = 0.05f;
        if (dev.on) dt = 1.0f / 30.0f;
        glfwPollEvents();

        fpsAcc += dt; ++fpsN;
        if (fpsAcc > 0.5f) { fps = fpsN / fpsAcc; fpsAcc = 0; fpsN = 0; }

        // --- entrada continua ---
        CamInput in;
        auto down = [&](int k) { return glfwGetKey(g.window, k) == GLFW_PRESS; };
        in.forward = (down(GLFW_KEY_W) ? 1.0f : 0.0f) - (down(GLFW_KEY_S) ? 1.0f : 0.0f);
        in.right = (down(GLFW_KEY_D) ? 1.0f : 0.0f) - (down(GLFW_KEY_A) ? 1.0f : 0.0f);
        in.up = (down(GLFW_KEY_SPACE) ? 1.0f : 0.0f) - (down(GLFW_KEY_LEFT_CONTROL) ? 1.0f : 0.0f);
        in.fast = down(GLFW_KEY_LEFT_SHIFT);
        in.mdx = g.mdx; in.mdy = g.mdy; in.scroll = g.scroll;
        g.mdx = g.mdy = g.scroll = 0;

        // --- actualización ---
        if (!g.paused)
        {
            g.time += dt;
            // hora del día con transición suave
            float dh = g.hourTarget - g.hour;
            if (dh > 12.0f) dh -= 24.0f;
            if (dh < -12.0f) dh += 24.0f;
            float step = U::clampf(dh, -4.0f * dt, 4.0f * dt);
            g.hour = std::fmod(g.hour + step + 24.0f, 24.0f);
            float fl = g.floodOn ? 1.0f : 0.0f;
            g.floodLevel += U::clampf(fl - g.floodLevel, -1.5f * dt, 1.5f * dt);
            float wv = g.wave ? 1.0f : 0.0f;
            g.waveOn += U::clampf(wv - g.waveOn, -0.8f * dt, 0.8f * dt);
            g.waveTime += dt;

            g_match.update(dt);
            for (size_t i = 0; i < g_match.events.size(); ++i)
            {
                int e = g_match.events[i];
                if (e == EV_HIT) Audio::play(SND_HIT);
                else if (e == EV_BOUNCE) Audio::play(SND_BOUNCE);
                else if (e == EV_APPLAUSE) Audio::play(SND_APPLAUSE);
            }
            g_match.events.clear();
        }
        g_match.players[0].hideHead = (g_cam.mode == CAM_JUGADOR);

        CamScene cs = buildCamScene();
        if (dev.on && dev.hasPos && shotFrames == 0) g_cam.setFree(dev.pos, dev.look);
        g_cam.update(dt, in, cs);

        // el marcador dinámico se sube a la GPU solo cuando cambia
        static int boardSeen = -1;
        if (boardSeen != g_match.boardVersion) { textures.updateScoreboard(g_match.board); boardSeen = g_match.boardVersion; }

        // --- iluminación y estado del estadio ---
        Lighting L = computeLighting(g.hour, g.floodLevel, g_stadium.anchors.towerLamps, g_stadium.anchors.points);
        StadiumDraw sd;
        sd.surface = g.surface; sd.time = g.time; sd.scorePulse = g_match.scorePulse; sd.cheer = g_match.cheer;
        sd.waveOn = g.waveOn; sd.waveTime = g.waveTime; sd.floodLevel = g.floodLevel;

        float aspect = static_cast<float>(g.fbW) / static_cast<float>(g.fbH);
        glm::mat4 proj = glm::perspective(U::rad(g_cam.fov), aspect, 0.1f, 700.0f);
        glm::mat4 view = g_cam.view();

        // --- 1) pasada de sombras ---
        if (g.wire) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        renderer.beginShadow(L);
        g_stadium.drawOpaque(renderer, sd, assets);
        g_match.draw(renderer, assets);
        renderer.endShadow(g.fbW, g.fbH);
        if (g.wire) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        // --- 2) escena ---
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderer.beginScene(view, proj, g_cam.eye(), L, g.time, g.fbW, g.fbH);
        if (!g.wire) renderer.drawSky(L, g.time);
        g_stadium.drawOpaque(renderer, sd, assets);
        g_match.draw(renderer, assets);
        g_stadium.drawTransparent(renderer, sd);

        // --- 3) HUD ---
        if (g.wire) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        if (!dev.on || dev.hasPos == false) drawHud(hud, fps);
        if (g.wire) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        ++shotFrames;
        if (dev.on)
        {
            saveScreenshot(dev.file);
            break;
        }
        glfwSwapBuffers(g.window);
    }

    glfwTerminate();
    return 0;
}
