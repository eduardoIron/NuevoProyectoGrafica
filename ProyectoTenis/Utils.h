// ============================================================================
//  Utils.h  -  Utilidades generales (matemáticas, aleatorios, archivos)
//  Proyecto Final - Escenario Deportivo 3D: Tenis
// ============================================================================
#pragma once

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace U
{
    const float PI = 3.14159265358979323846f;

    inline float rad(float d) { return d * PI / 180.0f; }
    inline float deg(float r) { return r * 180.0f / PI; }
    inline float clampf(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }
    inline float clamp01(float x) { return clampf(x, 0.0f, 1.0f); }
    inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
    inline float smooth(float t) { t = clamp01(t); return t * t * (3.0f - 2.0f * t); }
    inline float smoother(float t) { t = clamp01(t); return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }
    inline float sign(float x) { return x < 0.0f ? -1.0f : 1.0f; }

    // Envuelve un ángulo en grados a [-180, 180]
    inline float wrap180(float a)
    {
        while (a > 180.0f)  a -= 360.0f;
        while (a < -180.0f) a += 360.0f;
        return a;
    }

    // Acerca el ángulo "cur" a "target" (grados) avanzando como máximo "step" grados
    inline float approachAngle(float cur, float target, float step)
    {
        float d = wrap180(target - cur);
        if (d > step) d = step;
        if (d < -step) d = -step;
        return cur + d;
    }

    // Suavizado exponencial independiente de los FPS
    inline float damp(float rate, float dt) { return 1.0f - std::exp(-rate * dt); }

    // ---------- hash / ruido ----------
    inline uint32_t hash32(uint32_t x)
    {
        x ^= x >> 16; x *= 0x7feb352dU;
        x ^= x >> 15; x *= 0x846ca68bU;
        x ^= x >> 16;
        return x;
    }
    inline float hash01(uint32_t x) { return (hash32(x) & 0xFFFFFFu) / 16777216.0f; }
    inline float hash2(int x, int y, uint32_t seed = 0)
    {
        return hash01(static_cast<uint32_t>(x) * 73856093u ^ static_cast<uint32_t>(y) * 19349663u ^ (seed * 83492791u + 0x9e3779b9u));
    }

    // Generador pseudoaleatorio (xorshift32) reproducible
    struct Rng
    {
        uint32_t s;
        explicit Rng(uint32_t seed = 1u) : s(seed ? seed : 1u) {}
        uint32_t nextU() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
        float next() { return (nextU() & 0xFFFFFFu) / 16777216.0f; }
        float range(float a, float b) { return a + (b - a) * next(); }
        int irange(int a, int b) { return a + static_cast<int>(nextU() % static_cast<uint32_t>(b - a + 1)); }
        bool chance(float p) { return next() < p; }
    };

    // Ruido de valor "tileable": x,y en unidades de celda; el patrón se repite cada "period" celdas
    inline float valueNoise(float x, float y, int period, uint32_t seed = 0)
    {
        int ix = static_cast<int>(std::floor(x)), iy = static_cast<int>(std::floor(y));
        float fx = x - ix, fy = y - iy;
        fx = fx * fx * (3.0f - 2.0f * fx);
        fy = fy * fy * (3.0f - 2.0f * fy);
        int x0 = ((ix % period) + period) % period, x1 = (x0 + 1) % period;
        int y0 = ((iy % period) + period) % period, y1 = (y0 + 1) % period;
        float a = hash2(x0, y0, seed), b = hash2(x1, y0, seed);
        float c = hash2(x0, y1, seed), d = hash2(x1, y1, seed);
        return lerp(lerp(a, b, fx), lerp(c, d, fx), fy);
    }

    // fBm tileable. u,v en [0,1); basePeriod = celdas en la octava 0
    inline float fbm(float u, float v, int basePeriod, int octaves, uint32_t seed = 0)
    {
        float sum = 0.0f, amp = 0.5f, norm = 0.0f;
        int p = basePeriod;
        for (int o = 0; o < octaves; ++o)
        {
            sum += amp * valueNoise(u * p, v * p, p, seed + o * 17u);
            norm += amp;
            amp *= 0.5f;
            p *= 2;
        }
        return sum / norm;
    }

    // Interpolación Catmull-Rom (p1->p2) para trayectorias suaves
    inline glm::vec3 catmull(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t)
    {
        float t2 = t * t, t3 = t2 * t;
        return 0.5f * ((2.0f * p1) + (-p0 + p2) * t + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
    }

    // ---------- archivos ----------
    inline bool fileExists(const std::string& path)
    {
        FILE* f = std::fopen(path.c_str(), "rb");
        if (!f) return false;
        std::fclose(f);
        return true;
    }

    // Guarda una imagen RGB (filas de ARRIBA hacia ABAJO) como BMP de 24 bits
    inline bool saveBMP(const std::string& path, const unsigned char* rgb, int w, int h)
    {
        FILE* f = std::fopen(path.c_str(), "wb");
        if (!f) return false;
        int rowBytes = (w * 3 + 3) & ~3;
        uint32_t dataSize = static_cast<uint32_t>(rowBytes * h);
        uint32_t fileSize = 54 + dataSize;
        unsigned char hdr[54] = { 'B','M', 0,0,0,0, 0,0,0,0, 54,0,0,0, 40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0 };
        hdr[2] = fileSize & 255; hdr[3] = (fileSize >> 8) & 255; hdr[4] = (fileSize >> 16) & 255; hdr[5] = (fileSize >> 24) & 255;
        hdr[18] = w & 255; hdr[19] = (w >> 8) & 255; hdr[20] = (w >> 16) & 255; hdr[21] = (w >> 24) & 255;
        hdr[22] = h & 255; hdr[23] = (h >> 8) & 255; hdr[24] = (h >> 16) & 255; hdr[25] = (h >> 24) & 255;
        hdr[34] = dataSize & 255; hdr[35] = (dataSize >> 8) & 255; hdr[36] = (dataSize >> 16) & 255; hdr[37] = (dataSize >> 24) & 255;
        std::fwrite(hdr, 1, 54, f);
        std::vector<unsigned char> row(rowBytes, 0);
        for (int y = h - 1; y >= 0; --y)            // BMP guarda las filas de abajo hacia arriba
        {
            const unsigned char* src = rgb + static_cast<size_t>(y) * w * 3;
            for (int x = 0; x < w; ++x)
            {
                row[x * 3 + 0] = src[x * 3 + 2];    // BGR
                row[x * 3 + 1] = src[x * 3 + 1];
                row[x * 3 + 2] = src[x * 3 + 0];
            }
            std::fwrite(row.data(), 1, rowBytes, f);
        }
        std::fclose(f);
        return true;
    }
}
