#include "Model.h"
#include "Texturas.h"
#include "SOIL2/SOIL2.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <map>

namespace
{
    struct Loader
    {
        Model* model;
        std::string dir;
        std::map<std::string, std::pair<GLuint, bool> > cache;     // textura -> (id, tieneAlpha)

        GLuint texture(const std::string& file, bool& alpha)
        {
            std::map<std::string, std::pair<GLuint, bool> >::iterator it = cache.find(file);
            if (it != cache.end()) { alpha = it->second.second; return it->second.first; }
            std::string full = dir.empty() ? file : dir + "/" + file;
            bool a = false;
            GLuint id = loadTextureFile(full, &a);
            if (!id) std::cout << "AVISO::MODEL: no se pudo cargar la textura " << full << std::endl;
            cache[file] = std::make_pair(id, a);
            alpha = a;
            return id;
        }

        void processMesh(const aiMesh* m, const aiScene* scene)
        {
            std::vector<Vertex> verts;
            std::vector<unsigned int> idx;
            verts.reserve(m->mNumVertices);
            for (unsigned int i = 0; i < m->mNumVertices; ++i)
            {
                Vertex v;
                v.pos = glm::vec3(m->mVertices[i].x, m->mVertices[i].y, m->mVertices[i].z);
                v.nrm = m->HasNormals() ? glm::vec3(m->mNormals[i].x, m->mNormals[i].y, m->mNormals[i].z) : glm::vec3(0, 1, 0);
                v.uv = m->mTextureCoords[0] ? glm::vec2(m->mTextureCoords[0][i].x, m->mTextureCoords[0][i].y) : glm::vec2(0.0f);
                v.col = glm::vec4(1.0f);
                verts.push_back(v);
                model->bmin = glm::min(model->bmin, v.pos);
                model->bmax = glm::max(model->bmax, v.pos);
            }
            for (unsigned int f = 0; f < m->mNumFaces; ++f)
                for (unsigned int j = 0; j < m->mFaces[f].mNumIndices; ++j)
                    idx.push_back(m->mFaces[f].mIndices[j]);

            ModelPart part;
            if (m->mMaterialIndex < scene->mNumMaterials)
            {
                const aiMaterial* mat = scene->mMaterials[m->mMaterialIndex];
                aiColor3D kd(1.0f, 1.0f, 1.0f), ks(0.0f, 0.0f, 0.0f);
                float ns = 32.0f;
                mat->Get(AI_MATKEY_COLOR_DIFFUSE, kd);
                mat->Get(AI_MATKEY_COLOR_SPECULAR, ks);
                mat->Get(AI_MATKEY_SHININESS, ns);
                part.color = glm::vec3(kd.r, kd.g, kd.b);
                part.specular = U::clamp01((ks.r + ks.g + ks.b) / 3.0f);
                part.shininess = U::clampf(ns, 4.0f, 128.0f);
                if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0)
                {
                    aiString str;
                    mat->GetTexture(aiTextureType_DIFFUSE, 0, &str);
                    bool alpha = false;
                    part.tex = texture(str.C_Str(), alpha);
                    part.alphaTest = alpha;
                    if (part.tex) part.color = glm::vec3(1.0f);    // la textura manda
                }
            }
            part.mesh.upload(verts, idx);
            model->parts.push_back(part);
        }

        void processNode(const aiNode* node, const aiScene* scene)
        {
            for (unsigned int i = 0; i < node->mNumMeshes; ++i)
                processMesh(scene->mMeshes[node->mMeshes[i]], scene);
            for (unsigned int i = 0; i < node->mNumChildren; ++i)
                processNode(node->mChildren[i], scene);
        }
    };
}

bool Model::load(const std::string& path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);
    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
    {
        std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << " (" << path << ")" << std::endl;
        return false;
    }
    Loader L;
    L.model = this;
    size_t slash = path.find_last_of("/\\");
    L.dir = (slash == std::string::npos) ? "" : path.substr(0, slash);
    L.processNode(scene->mRootNode, scene);
    return true;
}

bool Assets::load()
{
    bool ok = true;
    ok &= racket.load("Models/raqueta.obj");
    ok &= ball.load("Models/pelota.obj");
    ok &= bottle.load("Models/botella.obj");
    ok &= towel.load("Models/toalla.obj");
    ok &= cat.load("Models/miGato.obj");       // modelos de tu práctica "Carga de Modelos"
    ok &= dog.load("Models/RedDog.obj");
    return ok;
}
