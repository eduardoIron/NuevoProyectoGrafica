// ============================================================================
//  Font.h  -  Fuente bitmap 5x7 (mayúsculas, dígitos y signos)
//  Se usa para: marcador electrónico, publicidad, letreros del túnel y HUD.
// ============================================================================
#pragma once

#include <GL/glew.h>
#include "Utils.h"
#include "Shader.h"
#include <string>
#include <vector>

namespace Font
{
    const int GLYPH_W = 5, GLYPH_H = 7;
    // Devuelve 7 filas; en cada fila los bits 4..0 son los píxeles de izquierda a derecha.
    const unsigned char* glyph(char c);
}

// Lienzo RGBA en CPU (fila 0 = arriba) para pintar texturas con texto
class Canvas
{
public:
    int w, h;
    std::vector<unsigned char> px;
    Canvas(int width, int height) : w(width), h(height), px(static_cast<size_t>(width) * height * 4, 0) {}

    void fill(const glm::vec4& c);
    void rect(int x, int y, int rw, int rh, const glm::vec4& c);
    // Dibuja texto; scale = tamaño de cada "pixel" de la fuente. Devuelve el ancho dibujado.
    int text(int x, int y, int scale, const std::string& s, const glm::vec4& c);
    static int textWidth(const std::string& s, int scale) { return static_cast<int>(s.size()) * 6 * scale - scale; }
};

// Texto en pantalla (HUD) con un atlas R8 de la fuente
class HudText
{
public:
    bool init();
    void begin(int screenW, int screenH);
    void text(float x, float y, float scale, const std::string& s, const glm::vec4& color, bool shadow = true);
    void rect(float x, float y, float w, float h, const glm::vec4& color);
    void end();
    float width(const std::string& s, float scale) const { return s.size() * 6.0f * scale - scale; }

private:
    Shader shader_;
    GLuint vao_ = 0, vbo_ = 0, tex_ = 0;
    int sw_ = 1, sh_ = 1;
    void flush(const std::vector<float>& verts, const glm::vec4& color);
    void emitGlyph(std::vector<float>& out, float x, float y, float scale, char c) const;
};
