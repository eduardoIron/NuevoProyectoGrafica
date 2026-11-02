// ============================================================================
//  Audio.h  -  Efectos de sonido sintetizados en memoria (sin archivos externos)
//  Windows: PlaySound (winmm). En otras plataformas no hace nada.
// ============================================================================
#pragma once

enum SoundId { SND_WHISTLE = 0, SND_HIT, SND_BOUNCE, SND_APPLAUSE, SND_COUNT };

namespace Audio
{
    void init();
    void play(SoundId id);
}
