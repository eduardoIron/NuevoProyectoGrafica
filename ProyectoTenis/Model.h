// ============================================================================
//  Model.h  -  Carga de modelos 3D (.obj / .glTF / ...) con Assimp
//  Evolución de las clases Model/Mesh de la práctica "Carga de Modelos":
//   - usa el mismo formato de vértice que el resto del proyecto
//   - lee el color difuso (Kd) y el brillo del .mtl además de la textura
//   - conserva la caja envolvente para poder escalar/colocar el modelo
// ============================================================================
#pragma once

#include <GL/glew.h>
#include "Utils.h"
#include "Mesh.h"
#include <string>
#include <vector>

struct ModelPart
{
    Mesh mesh;
    GLuint tex = 0;                           // 0 = sin textura
    glm::vec3 color = glm::vec3(1.0f);        // Kd del material
    float specular = 0.15f;
    float shininess = 32.0f;
    bool alphaTest = false;                   // la textura tiene transparencia (p. ej. cuerdas de la raqueta)
};

class Model
{
public:
    std::vector<ModelPart> parts;
    glm::vec3 bmin = glm::vec3(1e9f), bmax = glm::vec3(-1e9f);

    // Carga el archivo; devuelve false si Assimp falla.
    bool load(const std::string& path);
    glm::vec3 size() const { return bmax - bmin; }
};

// Conjunto de modelos externos que usa el proyecto (todos cargados con Assimp)
struct Assets
{
    Model racket, ball, bottle, towel, cat, dog;
    bool load();     // false si falta alguno (el programa avisa en consola y sigue)
};
