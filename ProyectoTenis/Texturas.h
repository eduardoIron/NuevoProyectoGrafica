// ============================================================================
//  Texturas.h  -  Texturas del escenario
//
//  La mayoría se generan por PROCEDIMIENTO al iniciar (ruido fBm tileable), así el
//  proyecto no depende de imágenes externas. Si colocas un archivo en la carpeta
//  Textures/ con alguno de estos nombres (png/jpg) se usa en lugar del procedural:
//      clay, grass, hard, concrete, asphalt
//  (su mapa de normales se calcula a partir de la luminancia de la imagen).
// ============================================================================
#pragma once

#include <GL/glew.h>
#include "Utils.h"
#include "Font.h"
#include <string>
#include <vector>

struct TexParams
{
    bool mipmaps = true;
    bool repeat = true;
    bool linear = true;
    float aniso = 8.0f;
    bool borderZero = false;     // fuera de [0,1] devuelve 0 (para la máscara de líneas)
};

// Crea una textura 2D a partir de datos en memoria (channels: 1,2,3 ó 4; fila 0 = arriba)
GLuint createTexture(int w, int h, int channels, const unsigned char* data, const TexParams& p);

// Carga una imagen con SOIL2 (devuelve 0 si no existe). hasAlpha indica si trae transparencia real.
GLuint loadTextureFile(const std::string& path, bool* hasAlpha = 0, bool srgbHint = false);

class Textures
{
public:
    GLuint white = 0;
    GLuint clay = 0, clayN = 0;
    GLuint grass = 0, grassN = 0;
    GLuint hard = 0, hardN = 0;
    GLuint concrete = 0, concreteN = 0;
    GLuint asphalt = 0, asphaltN = 0;
    GLuint lines = 0;        // decal de la cancha: R = líneas, G = zona interior
    GLuint seats = 0, ads = 0, net = 0, signs = 0;
    GLuint scoreboard = 0;   // textura dinámica del marcador (512x256 RGBA)

    // Región cubierta por la textura de líneas (metros, plano XZ)
    static const float DECAL_X0, DECAL_Z0, DECAL_W, DECAL_H;
    // Posición (px) de cada letrero dentro del atlas "signs": 4 filas de 64 px
    static const int SIGN_ROWS = 4;

    void generateAll();
    void updateScoreboard(const Canvas& c);
};
