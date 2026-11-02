// ============================================================================
//  Config.h  -  Dimensiones del recinto (metros) y parámetros del proyecto
//  Medidas de la cancha según reglamento ITF. Eje Z = largo de la cancha,
//  eje X = ancho, la red está en z = 0.
// ============================================================================
#pragma once

namespace Cfg
{
    // ---- Cancha de tenis (ITF) ----
    const float HL = 11.885f;          // mitad del largo (línea de fondo)
    const float HW = 5.485f;           // mitad del ancho de dobles
    const float SW = 4.115f;           // mitad del ancho de sencillos
    const float SERVICE_Z = 6.40f;     // línea de saque (distancia a la red)
    const float NET_POST_X = 6.40f;    // postes a 0.914 m fuera de la línea de dobles
    const float NET_H_POST = 1.07f;
    const float NET_H_CENTER = 0.914f;

    // ---- Piso de juego y muro de publicidad ----
    const float FLOOR_HX = 10.5f;
    const float FLOOR_HZ = 19.0f;
    const float WALL_H = 1.10f;

    // ---- Gradas laterales (oeste x<0, este x>0) ----
    const float SIDE_X0 = 12.0f;
    const float SIDE_HALF_Z = 20.0f;
    const int   SIDE_ROWS = 15;
    // ---- Gradas de fondo (sur z<0, norte z>0) ----
    const float END_Z0 = 20.5f;
    const float END_HALF_X = 12.0f;
    const int   END_ROWS = 11;
    const float ROW_DEPTH = 0.85f;
    const float ROW_RISE = 0.42f;

    // ---- Túnel de vestidores (en el fondo sur) ----
    const float TUNNEL_HALF_W = 2.0f;
    const float TUNNEL_WALL = 0.30f;
    const float TUNNEL_H = 2.8f;
    const float TUNNEL_Z0 = -19.0f;    // boca del túnel
    const float TUNNEL_Z1 = -29.5f;    // fondo del túnel

    // ---- Silla del juez ----
    const float UMPIRE_X = 7.6f;
    const float UMPIRE_SEAT_Y = 2.34f; // altura de la superficie del asiento

    // ---- Zona de descanso de jugadores (lado oeste) ----
    const float REST_X = -8.4f;

    // ---- Jugadores ----
    const float PLAYER_BASELINE_OFFSET = 0.9f;   // distancia detrás de la línea de fondo al sacar
    const float BALL_RADIUS = 0.0335f;

    // ---- Torres de iluminación ----
    const float TOWER_X = 22.0f;
    const float TOWER_Z = 26.0f;
    const float TOWER_H = 31.0f;
}
