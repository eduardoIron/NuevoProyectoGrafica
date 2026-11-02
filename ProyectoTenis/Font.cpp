#include "Font.h"
#include <cstring>

// ---------------------------------------------------------------------------
//  Tabla de glifos 5x7. Cada glifo son 7 filas de 5 bits (bit 4 = columna izquierda).
// ---------------------------------------------------------------------------
namespace
{
    struct GlyphDef { char ch; unsigned char rows[7]; };
    const GlyphDef GLYPHS[] = {
        { ' ', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },
        { '0', { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E } },
        { '1', { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E } },
        { '2', { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F } },
        { '3', { 0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E } },
        { '4', { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 } },
        { '5', { 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E } },
        { '6', { 0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E } },
        { '7', { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 } },
        { '8', { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E } },
        { '9', { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E } },
        { 'A', { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
        { 'B', { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E } },
        { 'C', { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E } },
        { 'D', { 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E } },
        { 'E', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F } },
        { 'F', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 } },
        { 'G', { 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F } },
        { 'H', { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
        { 'I', { 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E } },
        { 'J', { 0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C } },
        { 'K', { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 } },
        { 'L', { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F } },
        { 'M', { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 } },
        { 'N', { 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 } },
        { 'O', { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
        { 'P', { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 } },
        { 'Q', { 0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D } },
        { 'R', { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 } },
        { 'S', { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E } },
        { 'T', { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 } },
        { 'U', { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
        { 'V', { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04 } },
        { 'W', { 0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11 } },
        { 'X', { 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11 } },
        { 'Y', { 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04 } },
        { 'Z', { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F } },
        { '-', { 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 } },
        { '_', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F } },
        { '.', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C } },
        { ',', { 0x00, 0x00, 0x00, 0x00, 0x0C, 0x08, 0x10 } },
        { ':', { 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00 } },
        { '/', { 0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10 } },
        { '!', { 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04 } },
        { '?', { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04 } },
        { '+', { 0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00 } },
        { '*', { 0x00, 0x15, 0x0E, 0x1F, 0x0E, 0x15, 0x00 } },
        { '=', { 0x00, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00 } },
        { '>', { 0x10, 0x08, 0x04, 0x02, 0x04, 0x08, 0x10 } },
        { '<', { 0x01, 0x02, 0x04, 0x08, 0x04, 0x02, 0x01 } },
        { '(', { 0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02 } },
        { ')', { 0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08 } },
        { '%', { 0x19, 0x19, 0x02, 0x04, 0x08, 0x13, 0x13 } },
        { '[', { 0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E } },
        { ']', { 0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E } },
        { '\'', { 0x04, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00 } },
        { '#', { 0x0A, 0x1F, 0x0A, 0x0A, 0x1F, 0x0A, 0x00 } },
    };
    const int GLYPH_COUNT = sizeof(GLYPHS) / sizeof(GLYPHS[0]);
    unsigned char g_table[128][7];
    bool g_ready = false;

    void buildTable()
    {
        std::memset(g_table, 0, sizeof(g_table));
        for (int i = 0; i < GLYPH_COUNT; ++i)
            std::memcpy(g_table[static_cast<unsigned char>(GLYPHS[i].ch)], GLYPHS[i].rows, 7);
        // minúsculas = mayúsculas
        for (int c = 'a'; c <= 'z'; ++c) std::memcpy(g_table[c], g_table[c - 'a' + 'A'], 7);
        g_ready = true;
    }
}

const unsigned char* Font::glyph(char c)
{
    if (!g_ready) buildTable();
    unsigned char u = static_cast<unsigned char>(c);
    if (u >= 128) u = '?';
    return g_table[u];
}

// ---------------------------------------------------------------------------
//  Canvas (lienzo CPU)
// ---------------------------------------------------------------------------
static unsigned char toByte(float f) { return static_cast<unsigned char>(U::clamp01(f) * 255.0f + 0.5f); }

void Canvas::fill(const glm::vec4& c)
{
    for (int i = 0; i < w * h; ++i)
    {
        px[i * 4 + 0] = toByte(c.r); px[i * 4 + 1] = toByte(c.g);
        px[i * 4 + 2] = toByte(c.b); px[i * 4 + 3] = toByte(c.a);
    }
}

void Canvas::rect(int x, int y, int rw, int rh, const glm::vec4& c)
{
    for (int j = y; j < y + rh; ++j)
    {
        if (j < 0 || j >= h) continue;
        for (int i = x; i < x + rw; ++i)
        {
            if (i < 0 || i >= w) continue;
            unsigned char* p = &px[(static_cast<size_t>(j) * w + i) * 4];
            p[0] = toByte(c.r); p[1] = toByte(c.g); p[2] = toByte(c.b); p[3] = toByte(c.a);
        }
    }
}

int Canvas::text(int x, int y, int scale, const std::string& s, const glm::vec4& c)
{
    int cx = x;
    for (size_t k = 0; k < s.size(); ++k)
    {
        const unsigned char* g = Font::glyph(s[k]);
        for (int r = 0; r < 7; ++r)
            for (int b = 0; b < 5; ++b)
                if (g[r] & (1 << (4 - b))) rect(cx + b * scale, y + r * scale, scale, scale, c);
        cx += 6 * scale;
    }
    return cx - x - scale;
}

// ---------------------------------------------------------------------------
//  HudText (texto en pantalla)
// ---------------------------------------------------------------------------
// Atlas: 16 columnas x 8 filas de celdas de 8x8 px. La celda del caracter c esta en (c%16, c/16).
// La celda 127 se deja totalmente blanca para dibujar rectángulos.
bool HudText::init()
{
    if (!shader_.load("Shader/hud.vs", "Shader/hud.frag")) return false;
    const int AW = 128, AH = 64;
    std::vector<unsigned char> atlas(AW * AH, 0);
    for (int c = 32; c < 127; ++c)
    {
        const unsigned char* g = Font::glyph(static_cast<char>(c));
        int cx = (c % 16) * 8, cy = (c / 16) * 8;
        for (int r = 0; r < 7; ++r)
            for (int b = 0; b < 5; ++b)
                if (g[r] & (1 << (4 - b))) atlas[(cy + r) * AW + cx + b] = 255;
    }
    for (int j = 0; j < 8; ++j)
        for (int i = 0; i < 8; ++i) atlas[(7 * 8 + j) * AW + 15 * 8 + i] = 255;   // celda 127 sólida

    glGenTextures(1, &tex_);
    glBindTexture(GL_TEXTURE_2D, tex_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, AW, AH, 0, GL_RED, GL_UNSIGNED_BYTE, &atlas[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid*)0);
    glBindVertexArray(0);
    return true;
}

void HudText::begin(int screenW, int screenH)
{
    sw_ = screenW; sh_ = screenH;
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    shader_.use();
    shader_.setVec2("screen", glm::vec2(static_cast<float>(sw_), static_cast<float>(sh_)));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_);
    shader_.setInt("atlas", 0);
}

void HudText::end()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
}

void HudText::emitGlyph(std::vector<float>& o, float x, float y, float s, char ch) const
{
    unsigned char c = static_cast<unsigned char>(ch);
    if (c < 32 || c > 126) c = '?';
    if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 'a' + 'A');
    float u0 = ((c % 16) * 8) / 128.0f, v0 = ((c / 16) * 8) / 64.0f;
    float u1 = u0 + 6.0f / 128.0f, v1 = v0 + 8.0f / 64.0f;
    float x1 = x + 6.0f * s, y1 = y + 8.0f * s;
    float q[24] = { x, y, u0, v0,  x1, y, u1, v0,  x1, y1, u1, v1,
                    x, y, u0, v0,  x1, y1, u1, v1,  x, y1, u0, v1 };
    o.insert(o.end(), q, q + 24);
}

void HudText::flush(const std::vector<float>& verts, const glm::vec4& color)
{
    if (verts.empty()) return;
    shader_.setVec4("color", color);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), &verts[0], GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(verts.size() / 4));
    glBindVertexArray(0);
}

void HudText::text(float x, float y, float scale, const std::string& s, const glm::vec4& color, bool shadow)
{
    std::vector<float> v;
    if (shadow)
    {
        float o = scale > 1.5f ? scale * 0.5f : 1.0f;
        float cx = x + o;
        for (size_t i = 0; i < s.size(); ++i) { emitGlyph(v, cx, y + o, scale, s[i]); cx += 6.0f * scale; }
        flush(v, glm::vec4(0.0f, 0.0f, 0.0f, color.a * 0.85f));
        v.clear();
    }
    float cx = x;
    for (size_t i = 0; i < s.size(); ++i) { emitGlyph(v, cx, y, scale, s[i]); cx += 6.0f * scale; }
    flush(v, color);
}

void HudText::rect(float x, float y, float w, float h, const glm::vec4& color)
{
    // celda sólida (127): (15,7) del atlas; usamos su centro para evitar sangrado
    float u = (15 * 8 + 4) / 128.0f, v = (7 * 8 + 4) / 64.0f;
    float x1 = x + w, y1 = y + h;
    std::vector<float> q;
    float d[24] = { x, y, u, v,  x1, y, u, v,  x1, y1, u, v,  x, y, u, v,  x1, y1, u, v,  x, y1, u, v };
    q.insert(q.end(), d, d + 24);
    flush(q, color);
}
