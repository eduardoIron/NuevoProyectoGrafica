#include "Texturas.h"
#include "SOIL2/SOIL2.h"
#include <algorithm>
#include <cstring>
#include <iostream>

const float Textures::DECAL_X0 = -10.0f;
const float Textures::DECAL_Z0 = -18.0f;
const float Textures::DECAL_W = 20.0f;
const float Textures::DECAL_H = 36.0f;

// ---------------------------------------------------------------------------
GLuint createTexture(int w, int h, int channels, const unsigned char* data, const TexParams& p)
{
    GLuint t = 0;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    GLenum fmt = GL_RGBA, ifmt = GL_RGBA8;
    if (channels == 1) { fmt = GL_RED;  ifmt = GL_R8; }
    else if (channels == 2) { fmt = GL_RG;   ifmt = GL_RG8; }
    else if (channels == 3) { fmt = GL_RGB;  ifmt = GL_RGB8; }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, ifmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    GLenum wrap = p.repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    if (p.borderZero)
    {
        wrap = GL_CLAMP_TO_BORDER;
        float border[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    if (p.mipmaps)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, p.linear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST);
    }
    else
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, p.linear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, p.linear ? GL_LINEAR : GL_NEAREST);
    if (p.aniso > 1.0f && GLEW_EXT_texture_filter_anisotropic)
    {
        GLfloat maxA = 1.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxA);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, p.aniso < maxA ? p.aniso : maxA);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    return t;
}

GLuint loadTextureFile(const std::string& path, bool* hasAlpha, bool)
{
    int w = 0, h = 0, ch = 0;
    unsigned char* img = SOIL_load_image(path.c_str(), &w, &h, &ch, SOIL_LOAD_RGBA);
    if (!img) return 0;
    bool alpha = false;
    if (ch == 4)
        for (int i = 0; i < w * h; ++i)
            if (img[i * 4 + 3] < 250) { alpha = true; break; }
    if (hasAlpha) *hasAlpha = alpha;
    TexParams p;
    GLuint t = createTexture(w, h, 4, img, p);
    SOIL_free_image_data(img);
    return t;
}

// ---------------------------------------------------------------------------
//  Superficies procedurales
// ---------------------------------------------------------------------------
namespace
{
    struct Image
    {
        int w, h, ch;
        std::vector<unsigned char> d;
        Image(int W = 0, int H = 0, int C = 3) : w(W), h(H), ch(C), d(static_cast<size_t>(W) * H * C, 0) {}
    };

    unsigned char b8(float f) { return static_cast<unsigned char>(U::clamp01(f) * 255.0f + 0.5f); }

    // Mapa de normales a partir de una altura (tileable)
    Image normalFromHeight(const std::vector<float>& hgt, int N, float strength)
    {
        Image out(N, N, 3);
        for (int y = 0; y < N; ++y)
            for (int x = 0; x < N; ++x)
            {
                int xl = (x + N - 1) % N, xr = (x + 1) % N, yu = (y + N - 1) % N, yd = (y + 1) % N;
                float dx = (hgt[y * N + xr] - hgt[y * N + xl]) * strength;
                float dy = (hgt[yd * N + x] - hgt[yu * N + x]) * strength;
                glm::vec3 n = glm::normalize(glm::vec3(-dx, -dy, 1.0f));
                unsigned char* p = &out.d[(y * N + x) * 3];
                p[0] = b8(n.x * 0.5f + 0.5f); p[1] = b8(n.y * 0.5f + 0.5f); p[2] = b8(n.z * 0.5f + 0.5f);
            }
        return out;
    }

    enum SurfaceKind { S_CLAY, S_GRASS, S_HARD, S_CONCRETE, S_ASPHALT };

    void genSurface(SurfaceKind kind, int N, Image& diff, std::vector<float>& height)
    {
        diff = Image(N, N, 3);
        height.assign(static_cast<size_t>(N) * N, 0.5f);
        for (int y = 0; y < N; ++y)
            for (int x = 0; x < N; ++x)
            {
                float u = static_cast<float>(x) / N, v = static_cast<float>(y) / N;
                float g = U::hash2(x, y, 11u + kind);
                glm::vec3 c(0.5f);
                float hh = 0.5f;
                switch (kind)
                {
                case S_CLAY:
                {
                    float low = U::fbm(u, v, 4, 4, 3);
                    float mid = U::fbm(u, v, 32, 2, 5);
                    c = glm::vec3(0.76f, 0.37f, 0.22f) * (0.80f + 0.40f * low) * (0.93f + 0.14f * mid) * (0.93f + 0.14f * g);
                    float sp = U::hash2(x, y, 99u);
                    if (sp > 0.993f) c += glm::vec3(0.10f, 0.08f, 0.06f);
                    else if (sp < 0.006f) c *= 0.72f;
                    hh = 0.45f * mid + 0.55f * g;
                    break;
                }
                case S_GRASS:
                {
                    float n = U::fbm(u, v, 64, 3, 7);
                    float stripe = (std::fmod(v * 2.0f, 1.0f) < 0.5f) ? 0.86f : 1.10f;      // franjas de corte
                    glm::vec3 dark(0.10f, 0.32f, 0.07f), light(0.30f, 0.58f, 0.14f);
                    c = glm::mix(dark, light, n) * stripe * (0.86f + 0.28f * g);
                    hh = 0.5f * n + 0.5f * g;
                    break;
                }
                case S_HARD:
                {
                    float low = U::fbm(u, v, 4, 3, 13);
                    float mid = U::fbm(u, v, 48, 2, 15);
                    c = glm::vec3(0.15f, 0.33f, 0.60f) * (0.92f + 0.14f * low) * (0.96f + 0.08f * mid) * (0.96f + 0.08f * g);
                    hh = 0.6f * g + 0.4f * mid;
                    break;
                }
                case S_CONCRETE:
                {
                    float low = U::fbm(u, v, 4, 4, 21);
                    float mid = U::fbm(u, v, 24, 3, 23);
                    float k = 0.58f * (0.80f + 0.35f * low) * (0.92f + 0.16f * mid) * (0.94f + 0.12f * g);
                    c = glm::vec3(k, k * 0.99f, k * 0.96f);
                    hh = 0.55f * mid + 0.45f * g;
                    int half = N / 2;
                    if ((x % half) < 2 || (y % half) < 2) { c *= 0.55f; hh = 0.1f; }          // juntas de dilatación
                    if (U::hash2(x, y, 77u) > 0.995f) { c *= 0.6f; hh = 0.2f; }              // poros
                    break;
                }
                case S_ASPHALT:
                {
                    float low = U::fbm(u, v, 6, 4, 31);
                    float k = 0.20f * (0.75f + 0.5f * low) * (0.82f + 0.36f * g);
                    c = glm::vec3(k, k, k * 1.04f);
                    if (U::hash2(x, y, 55u) > 0.975f) c += glm::vec3(0.18f);
                    hh = 0.5f * low + 0.5f * g;
                    break;
                }
                }
                unsigned char* p = &diff.d[(y * N + x) * 3];
                p[0] = b8(c.r); p[1] = b8(c.g); p[2] = b8(c.b);
                height[y * N + x] = hh;
            }
    }

    // Si existe Textures/<name>.png|jpg|jpeg|bmp|tga lo usa (reemplaza lo procedural)
    bool tryOverride(const char* name, Image& diff, std::vector<float>& height)
    {
        static const char* ext[] = { ".png", ".jpg", ".jpeg", ".bmp", ".tga" };
        for (int e = 0; e < 5; ++e)
        {
            std::string path = std::string("Textures/") + name + ext[e];
            if (!U::fileExists(path)) continue;
            int w = 0, h = 0, ch = 0;
            unsigned char* img = SOIL_load_image(path.c_str(), &w, &h, &ch, SOIL_LOAD_RGB);
            if (!img) continue;
            diff = Image(w, h, 3);
            std::memcpy(&diff.d[0], img, static_cast<size_t>(w) * h * 3);
            height.assign(static_cast<size_t>(w) * h, 0.5f);
            for (int i = 0; i < w * h; ++i)
                height[i] = (0.3f * img[i * 3] + 0.59f * img[i * 3 + 1] + 0.11f * img[i * 3 + 2]) / 255.0f;
            SOIL_free_image_data(img);
            std::cout << "  [textura] usando archivo " << path << std::endl;
            return true;
        }
        return false;
    }

    // Normal map desde una altura de tamaño arbitrario (cuadrada o no)
    Image normalFromHeightRect(const std::vector<float>& hgt, int W, int H, float strength)
    {
        Image out(W, H, 3);
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
            {
                int xl = (x + W - 1) % W, xr = (x + 1) % W, yu = (y + H - 1) % H, yd = (y + 1) % H;
                float dx = (hgt[y * W + xr] - hgt[y * W + xl]) * strength;
                float dy = (hgt[yd * W + x] - hgt[yu * W + x]) * strength;
                glm::vec3 n = glm::normalize(glm::vec3(-dx, -dy, 1.0f));
                unsigned char* p = &out.d[(y * W + x) * 3];
                p[0] = b8(n.x * 0.5f + 0.5f); p[1] = b8(n.y * 0.5f + 0.5f); p[2] = b8(n.z * 0.5f + 0.5f);
            }
        return out;
    }

    void makeSurface(const char* name, SurfaceKind kind, int N, float nStrength, GLuint& texD, GLuint& texN)
    {
        Image diff;
        std::vector<float> hgt;
        Image nrm;
        if (tryOverride(name, diff, hgt))
            nrm = normalFromHeightRect(hgt, diff.w, diff.h, nStrength * 0.6f);
        else
        {
            genSurface(kind, N, diff, hgt);
            nrm = normalFromHeight(hgt, N, nStrength);
        }
        TexParams p;
        texD = createTexture(diff.w, diff.h, 3, &diff.d[0], p);
        TexParams pn = p;
        texN = createTexture(nrm.w, nrm.h, 3, &nrm.d[0], pn);
    }

    // Rectángulo antialias en una máscara (coordenadas en metros)
    void fillRectAA(std::vector<unsigned char>& buf, int W, int H, int channels, int chan, float ppm,
                    float x0, float x1, float z0, float z1)
    {
        float px0 = (x0 - Textures::DECAL_X0) * ppm, px1 = (x1 - Textures::DECAL_X0) * ppm;
        float py0 = (z0 - Textures::DECAL_Z0) * ppm, py1 = (z1 - Textures::DECAL_Z0) * ppm;
        int ix0 = static_cast<int>(std::floor(px0)), ix1 = static_cast<int>(std::ceil(px1));
        int iy0 = static_cast<int>(std::floor(py0)), iy1 = static_cast<int>(std::ceil(py1));
        for (int j = iy0; j < iy1; ++j)
        {
            if (j < 0 || j >= H) continue;
            float cy = U::clamp01(std::min(static_cast<float>(j + 1), py1) - std::max(static_cast<float>(j), py0));
            for (int i = ix0; i < ix1; ++i)
            {
                if (i < 0 || i >= W) continue;
                float cx = U::clamp01(std::min(static_cast<float>(i + 1), px1) - std::max(static_cast<float>(i), px0));
                unsigned char val = b8(cx * cy);
                unsigned char& dst = buf[(static_cast<size_t>(j) * W + i) * channels + chan];
                if (val > dst) dst = val;
            }
        }
    }

    GLuint genCourtLines()
    {
        const float ppm = 50.0f;
        const int W = static_cast<int>(Textures::DECAL_W * ppm), H = static_cast<int>(Textures::DECAL_H * ppm);
        std::vector<unsigned char> buf(static_cast<size_t>(W) * H * 2, 0);

        const float HL = 11.885f, HW = 5.485f, SW = 4.115f, SZ = 6.40f;
        const float lw = 0.05f, bw = 0.10f;
        // zona interior (color distinto en cancha dura)
        fillRectAA(buf, W, H, 2, 1, ppm, -8.5f, 8.5f, -16.8f, 16.8f);
        // líneas de fondo
        fillRectAA(buf, W, H, 2, 0, ppm, -HW, HW, HL - bw, HL);
        fillRectAA(buf, W, H, 2, 0, ppm, -HW, HW, -HL, -HL + bw);
        // laterales de dobles
        fillRectAA(buf, W, H, 2, 0, ppm, HW - lw, HW, -HL, HL);
        fillRectAA(buf, W, H, 2, 0, ppm, -HW, -HW + lw, -HL, HL);
        // laterales de sencillos
        fillRectAA(buf, W, H, 2, 0, ppm, SW - lw, SW, -SZ, SZ);
        fillRectAA(buf, W, H, 2, 0, ppm, -SW, -SW + lw, -SZ, SZ);
        // lineas de saque
        fillRectAA(buf, W, H, 2, 0, ppm, -SW, SW, SZ - lw, SZ);
        fillRectAA(buf, W, H, 2, 0, ppm, -SW, SW, -SZ, -SZ + lw);
        // línea central de saque
        fillRectAA(buf, W, H, 2, 0, ppm, -lw * 0.5f, lw * 0.5f, -SZ, SZ);
        // marcas centrales
        fillRectAA(buf, W, H, 2, 0, ppm, -lw * 0.5f, lw * 0.5f, HL - bw - 0.10f, HL - bw);
        fillRectAA(buf, W, H, 2, 0, ppm, -lw * 0.5f, lw * 0.5f, -HL + bw, -HL + bw + 0.10f);

        TexParams p;
        p.repeat = false;
        p.borderZero = true;
        p.aniso = 16.0f;
        return createTexture(W, H, 2, &buf[0], p);
    }

    // ---------------------------------------------------------- asientos
    GLuint genSeats()
    {
        const int W = 256, H = 128;
        Canvas c(W, H);
        c.fill(glm::vec4(0.16f, 0.16f, 0.17f, 1.0f));
        for (int s = 0; s < 2; ++s)
        {
            int x0 = s * 128 + 10, x1 = s * 128 + 118, y0 = 8, y1 = 120;
            for (int y = y0; y < y1; ++y)
                for (int x = x0; x < x1; ++x)
                {
                    // esquinas redondeadas
                    float rx = std::min(x - x0, x1 - 1 - x), ry = std::min(y - y0, y1 - 1 - y);
                    if (rx < 10 && ry < 10)
                    {
                        float dx = 10 - rx, dy = 10 - ry;
                        if (dx * dx + dy * dy > 100.0f) continue;
                    }
                    float t = static_cast<float>(y - y0) / (y1 - y0);
                    float k = 1.0f - 0.28f * t;                          // degradado vertical
                    float rib = (((x - x0) / 9) % 2 == 0) ? 1.0f : 0.93f;    // nervaduras
                    c.rect(x, y, 1, 1, glm::vec4(k * rib, k * rib, k * rib, 1.0f));
                }
        }
        TexParams p;
        return createTexture(W, H, 4, &c.px[0], p);
    }

    // ---------------------------------------------------------- publicidad
    GLuint genAds()
    {
        const int PW = 512, PH = 128, NP = 8;
        Canvas c(PW * NP, PH);
        struct Ad { const char* text; glm::vec3 bg, fg, accent; };
        Ad ads[NP] = {
            { "GRAFICA 3D", glm::vec3(0.05f, 0.25f, 0.65f), glm::vec3(1.0f), glm::vec3(1.0f, 0.8f, 0.1f) },
            { "TENIS",      glm::vec3(0.05f, 0.05f, 0.06f), glm::vec3(1.0f, 0.85f, 0.1f), glm::vec3(0.9f, 0.2f, 0.2f) },
            { "SMASH",      glm::vec3(0.80f, 0.10f, 0.12f), glm::vec3(1.0f), glm::vec3(1.0f) },
            { "AQUAFIT",    glm::vec3(0.00f, 0.55f, 0.58f), glm::vec3(1.0f), glm::vec3(0.8f, 1.0f, 0.2f) },
            { "OPENGL",     glm::vec3(0.28f, 0.10f, 0.45f), glm::vec3(1.0f), glm::vec3(0.4f, 0.9f, 1.0f) },
            { "RACKETPRO",  glm::vec3(0.95f, 0.50f, 0.05f), glm::vec3(0.05f), glm::vec3(1.0f) },
            { "ACE",        glm::vec3(0.05f, 0.45f, 0.18f), glm::vec3(1.0f), glm::vec3(1.0f, 1.0f, 0.3f) },
            { "TIEBREAK",   glm::vec3(0.04f, 0.10f, 0.30f), glm::vec3(1.0f), glm::vec3(0.9f, 0.3f, 0.3f) } };
        for (int i = 0; i < NP; ++i)
        {
            int x0 = i * PW;
            c.rect(x0, 0, PW, PH, glm::vec4(ads[i].bg, 1.0f));
            c.rect(x0, 0, PW, 8, glm::vec4(ads[i].accent, 1.0f));
            c.rect(x0, PH - 8, PW, 8, glm::vec4(ads[i].accent, 1.0f));
            c.rect(x0, 0, 4, PH, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));              // separación entre paneles
            std::string t = ads[i].text;
            int scale = 8;
            while (Canvas::textWidth(t, scale) > PW - 40) --scale;
            int tw = Canvas::textWidth(t, scale);
            c.text(x0 + (PW - tw) / 2, (PH - 7 * scale) / 2, scale, t, glm::vec4(ads[i].fg, 1.0f));
        }
        TexParams p;
        return createTexture(c.w, c.h, 4, &c.px[0], p);
    }

    // ---------------------------------------------------------- red
    GLuint genNet()
    {
        const int N = 256;
        Canvas c(N, N);
        c.fill(glm::vec4(0.0f));
        for (int k = 0; k < N; k += 16)
        {
            c.rect(k, 0, 2, N, glm::vec4(0.08f, 0.08f, 0.09f, 1.0f));
            c.rect(0, k, N, 2, glm::vec4(0.08f, 0.08f, 0.09f, 1.0f));
        }
        TexParams p;
        return createTexture(N, N, 4, &c.px[0], p);
    }

    // ---------------------------------------------------------- letreros
    GLuint genSigns()
    {
        Canvas c(512, 64 * Textures::SIGN_ROWS);
        struct Sg { const char* t; glm::vec3 bg, fg; };
        Sg s[Textures::SIGN_ROWS] = {
            { "VESTIDORES",        glm::vec3(0.05f, 0.30f, 0.15f), glm::vec3(1.0f) },
            { "LOCAL",             glm::vec3(0.10f, 0.20f, 0.60f), glm::vec3(1.0f) },
            { "VISITA",            glm::vec3(0.65f, 0.12f, 0.12f), glm::vec3(1.0f) },
            { "ENTRADA JUGADORES", glm::vec3(0.06f, 0.06f, 0.07f), glm::vec3(1.0f, 0.85f, 0.1f) } };
        for (int i = 0; i < Textures::SIGN_ROWS; ++i)
        {
            c.rect(0, i * 64, 512, 64, glm::vec4(s[i].bg, 1.0f));
            c.rect(0, i * 64, 512, 3, glm::vec4(1.0f));
            c.rect(0, i * 64 + 61, 512, 3, glm::vec4(1.0f));
            std::string t = s[i].t;
            int scale = 7;
            while (Canvas::textWidth(t, scale) > 490 || 7 * scale > 50) --scale;
            c.text((512 - Canvas::textWidth(t, scale)) / 2, i * 64 + (64 - 7 * scale) / 2, scale, t, glm::vec4(s[i].fg, 1.0f));
        }
        TexParams p;
        p.repeat = false;
        return createTexture(c.w, c.h, 4, &c.px[0], p);
    }
}

// ---------------------------------------------------------------------------
void Textures::generateAll()
{
    std::cout << "Generando texturas procedurales..." << std::endl;

    unsigned char w4[4] = { 255, 255, 255, 255 };
    TexParams pw;
    pw.mipmaps = false;
    white = createTexture(1, 1, 4, w4, pw);

    makeSurface("clay", S_CLAY, 512, 2.2f, clay, clayN);
    makeSurface("grass", S_GRASS, 512, 2.6f, grass, grassN);
    makeSurface("hard", S_HARD, 512, 1.2f, hard, hardN);
    makeSurface("concrete", S_CONCRETE, 512, 2.0f, concrete, concreteN);
    makeSurface("asphalt", S_ASPHALT, 256, 2.0f, asphalt, asphaltN);

    lines = genCourtLines();
    seats = genSeats();
    ads = genAds();
    net = genNet();
    signs = genSigns();

    // marcador: textura dinámica
    {
        Canvas c(512, 256);
        c.fill(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        TexParams p;
        p.mipmaps = false;
        p.repeat = false;
        p.aniso = 1.0f;
        scoreboard = createTexture(c.w, c.h, 4, &c.px[0], p);
    }
}

void Textures::updateScoreboard(const Canvas& c)
{
    glBindTexture(GL_TEXTURE_2D, scoreboard);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, c.w, c.h, GL_RGBA, GL_UNSIGNED_BYTE, &c.px[0]);
    glBindTexture(GL_TEXTURE_2D, 0);
}
