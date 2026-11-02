#include "Audio.h"
#include "Utils.h"
#include <vector>
#include <cstring>
#include <cstdint>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

namespace
{
    const int RATE = 22050;
    std::vector<unsigned char> g_wav[SND_COUNT];

    // Empaqueta muestras (float -1..1) en un WAV PCM 16 bits mono
    std::vector<unsigned char> makeWav(const std::vector<float>& s)
    {
        uint32_t dataBytes = static_cast<uint32_t>(s.size() * 2);
        std::vector<unsigned char> w(44 + dataBytes);
        unsigned char* p = &w[0];
        std::memcpy(p, "RIFF", 4);
        uint32_t v = 36 + dataBytes; std::memcpy(p + 4, &v, 4);
        std::memcpy(p + 8, "WAVEfmt ", 8);
        v = 16; std::memcpy(p + 16, &v, 4);
        uint16_t fmt = 1, ch = 1; std::memcpy(p + 20, &fmt, 2); std::memcpy(p + 22, &ch, 2);
        v = RATE; std::memcpy(p + 24, &v, 4);
        v = RATE * 2; std::memcpy(p + 28, &v, 4);
        uint16_t ba = 2, bits = 16; std::memcpy(p + 32, &ba, 2); std::memcpy(p + 34, &bits, 2);
        std::memcpy(p + 36, "data", 4);
        std::memcpy(p + 40, &dataBytes, 4);
        for (size_t i = 0; i < s.size(); ++i)
        {
            int16_t q = static_cast<int16_t>(U::clampf(s[i], -1.0f, 1.0f) * 30000.0f);
            std::memcpy(p + 44 + i * 2, &q, 2);
        }
        return w;
    }

    std::vector<float> whistle()
    {
        std::vector<float> s(static_cast<size_t>(RATE * 0.85f));
        for (size_t i = 0; i < s.size(); ++i)
        {
            float t = static_cast<float>(i) / RATE;
            float env = U::smooth(t / 0.03f) * (1.0f - U::smooth((t - 0.70f) / 0.15f));
            float trill = std::sin(2.0f * U::PI * 32.0f * t);                          // la "bolita" del silbato
            float f = 2900.0f + 60.0f * std::sin(2.0f * U::PI * 5.0f * t);
            float ph = 2.0f * U::PI * f * t + 3.0f * trill;
            s[i] = 0.45f * env * (std::sin(ph) + 0.35f * std::sin(2.0f * ph));
        }
        return s;
    }
    std::vector<float> hit()
    {
        std::vector<float> s(static_cast<size_t>(RATE * 0.16f));
        U::Rng r(7);
        for (size_t i = 0; i < s.size(); ++i)
        {
            float t = static_cast<float>(i) / RATE;
            float noise = (r.next() * 2.0f - 1.0f) * std::exp(-t * 90.0f);
            float tone = std::sin(2.0f * U::PI * 850.0f * t) * std::exp(-t * 45.0f);
            s[i] = 0.7f * (0.55f * noise + 0.6f * tone);
        }
        return s;
    }
    std::vector<float> bounce()
    {
        std::vector<float> s(static_cast<size_t>(RATE * 0.12f));
        for (size_t i = 0; i < s.size(); ++i)
        {
            float t = static_cast<float>(i) / RATE;
            s[i] = 0.55f * std::sin(2.0f * U::PI * (330.0f - 700.0f * t) * t) * std::exp(-t * 38.0f);
        }
        return s;
    }
    std::vector<float> applause()
    {
        std::vector<float> s(static_cast<size_t>(RATE * 2.6f), 0.0f);
        U::Rng r(21);
        float lp = 0.0f;
        for (size_t i = 0; i < s.size(); ++i)
        {
            float t = static_cast<float>(i) / RATE;
            float env = U::smooth(t / 0.25f) * (1.0f - U::smooth((t - 1.7f) / 0.9f));
            float n = r.next() * 2.0f - 1.0f;
            lp += (n - lp) * 0.45f;                                                     // ruido filtrado
            float clap = (r.chance(0.012f) ? 1.0f : 0.0f) * (r.next() * 2.0f - 1.0f);  // palmadas
            s[i] = env * (0.35f * lp + 0.5f * clap);
        }
        return s;
    }
}

void Audio::init()
{
    g_wav[SND_WHISTLE] = makeWav(whistle());
    g_wav[SND_HIT] = makeWav(hit());
    g_wav[SND_BOUNCE] = makeWav(bounce());
    g_wav[SND_APPLAUSE] = makeWav(applause());
}

void Audio::play(SoundId id)
{
#if defined(_WIN32)
    if (id < 0 || id >= SND_COUNT || g_wav[id].empty()) return;
    PlaySoundA(reinterpret_cast<LPCSTR>(&g_wav[id][0]), NULL, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
#else
    (void)id;
#endif
}
