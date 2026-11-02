// ============================================================================
//  Mesh.h  -  Malla en GPU (VAO/VBO/EBO) y constructor de primitivas
//
//  Formato de vértice unificado (también para modelos cargados con Assimp):
//     location 0: posición (vec3)
//     location 1: normal   (vec3)
//     location 2: uv       (vec2)   (convención imagen: v=0 arriba)
//     location 3: color    (vec4)   rgb = color de vértice, a = factor de tinte
//                                   (lo usa el shader instanciado del público)
//     location 4,5: datos por instancia (público)
// ============================================================================
#pragma once

#include <GL/glew.h>
#include "Utils.h"
#include <vector>

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 nrm;
    glm::vec2 uv;
    glm::vec4 col;
};

class Mesh
{
public:
    GLuint vao = 0, vbo = 0, ebo = 0, instVbo = 0;
    GLsizei count = 0;

    void upload(const std::vector<Vertex>& v, const std::vector<unsigned int>& idx);
    // Crea el buffer de instancias (8 floats por instancia: vec4 en loc 4 y vec4 en loc 5)
    void setInstances(const std::vector<float>& data);

    void draw() const;
    void drawInstanced(int n) const;
    bool valid() const { return vao != 0; }
};

// Construye geometría en CPU. Todas las primitivas aceptan una transformación
// rígida xf (traslación/rotación) que se aplica a posiciones y normales.
// Las coordenadas de textura se expresan en METROS divididos entre "tile":
// una textura con tile=(2,2) se repite cada 2 m sin importar el tamaño de la pieza.
class MeshBuilder
{
public:
    std::vector<Vertex> v;
    std::vector<unsigned int> idx;

    typedef glm::vec2 V2;
    typedef glm::vec3 V3;
    typedef glm::vec4 V4;
    typedef glm::mat4 M4;

    // Caja centrada en el origen de xf
    void box(const M4& xf, const V3& size, const V2& tile = V2(1.0f), const V4& col = V4(1.0f));
    // Caja alineada a los ejes entre mn y mx
    void boxAt(const V3& mn, const V3& mx, const V2& tile = V2(1.0f), const V4& col = V4(1.0f));
    // Plano horizontal (normal +Y) centrado en el origen de xf
    void plane(const M4& xf, float sx, float sz, const V2& tile = V2(1.0f), const V4& col = V4(1.0f));
    // Rectángulo vertical en XY mirando a +Z, centrado. uv de (0,0) arriba-izq a uvMax abajo-der
    void quad(const M4& xf, float w, float h, const V2& uvMax = V2(1.0f), const V4& col = V4(1.0f), bool doubleSide = false);
    // Cilindro/cono truncado a lo largo de +Y (de y=0 a y=h)
    void cylinder(const M4& xf, float rBottom, float rTop, float h, int segs = 16, bool caps = true,
                  const V2& tile = V2(1.0f), const V4& col = V4(1.0f));
    void sphere(const M4& xf, float r, int segs = 16, int rings = 10, const V4& col = V4(1.0f));
    // Barra cilíndrica entre dos puntos cualesquiera (patas, escaleras, postes inclinados)
    void bar(const V3& p0, const V3& p1, float radius, int segs = 8, const V4& col = V4(1.0f));
    // Copia otra malla transformada
    void append(const MeshBuilder& other, const M4& xf);

    Mesh build() const;
    void clear() { v.clear(); idx.clear(); }

private:
    unsigned int addV(const M4& xf, const V3& p, const V3& n, const V2& uv, const V4& col);
    void tri(unsigned int a, unsigned int b, unsigned int c);
};
