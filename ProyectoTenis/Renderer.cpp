#include "Renderer.h"
#include "Texturas.h"
#include <cstdlib>
#include <iostream>

// ---------------------------------------------------------------------------
//  Ciclo día/noche y luces
// ---------------------------------------------------------------------------
Lighting computeLighting(float hour, float flood, const std::vector<glm::vec3>& towerLamps,
                         const std::vector<PointLight>& points)
{
    Lighting L;
    float th = (hour - 6.0f) / 12.0f * U::PI;               // 0 = amanecer (6h), pi/2 = mediodía
    float elev = std::sin(th);
    glm::vec3 sun = glm::normalize(glm::vec3(std::cos(th) * 0.85f, elev, 0.52f));
    glm::vec3 moon = glm::normalize(glm::vec3(-std::cos(th) * 0.70f, -elev + 0.15f, 0.40f));
    if (moon.y < 0.25f) moon = glm::normalize(glm::vec3(moon.x, 0.25f, moon.z));
    float day = U::smooth((elev + 0.10f) / 0.35f);

    L.sunDir = sun;
    L.moonDir = moon;
    L.dayFactor = day;

    glm::vec3 warm(1.0f, 0.55f, 0.30f), white(1.0f, 0.96f, 0.90f);
    glm::vec3 sunCol = glm::mix(warm, white, U::smooth(elev / 0.55f)) * 3.3f * day;
    glm::vec3 moonCol = glm::vec3(0.32f, 0.42f, 0.70f) * 0.55f * (1.0f - day);
    L.lightDir = glm::normalize(glm::mix(moon, sun, day));
    L.lightColor = sunCol + moonCol;

    L.ambSky = glm::mix(glm::vec3(0.030f, 0.045f, 0.095f), glm::vec3(0.46f, 0.56f, 0.74f), day);
    L.ambGround = glm::mix(glm::vec3(0.020f, 0.022f, 0.030f), glm::vec3(0.30f, 0.28f, 0.24f), day);
    L.fogColor = glm::mix(glm::vec3(0.012f, 0.020f, 0.045f), glm::vec3(0.58f, 0.70f, 0.88f), day);
    // atardecer: niebla cálida
    float tw = U::smooth((0.30f - std::fabs(elev - 0.0f)) / 0.30f) * (day > 0.02f ? 1.0f : 0.0f);
    L.fogColor = glm::mix(L.fogColor, glm::vec3(0.75f, 0.40f, 0.22f), tw * 0.5f);
    L.fogDensity = 0.0022f;
    L.exposure = U::lerp(1.05f, 1.0f, day);

    // matriz de luz (ortográfica, centrada en la cancha)
    glm::vec3 center(0.0f, 2.0f, 0.0f);
    glm::vec3 up = std::fabs(L.lightDir.y) > 0.98f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
    glm::mat4 lv = glm::lookAt(center + L.lightDir * 90.0f, center, up);
    const float R = 56.0f;
    glm::mat4 lp = glm::ortho(-R, R, -R, R, 5.0f, 190.0f);
    L.lightSpace = lp * lv;

    // reflectores del estadio (spots dirigidos a la cancha)
    L.floodLevel = flood;
    L.nSpots = static_cast<int>(towerLamps.size() > 4 ? 4 : towerLamps.size());
    for (int i = 0; i < L.nSpots; ++i)
    {
        glm::vec3 p = towerLamps[i];
        glm::vec3 target(-U::sign(p.x) * 2.5f, 0.0f, -U::sign(p.z) * 4.0f);
        L.spots[i].pos = p;
        L.spots[i].dir = glm::normalize(target - p);
        L.spots[i].color = glm::vec3(1.0f, 0.95f, 0.85f) * (2.3f * flood);
        L.spots[i].innerCos = std::cos(U::rad(17.0f));
        L.spots[i].outerCos = std::cos(U::rad(33.0f));
    }
    L.nPoints = static_cast<int>(points.size() > 3 ? 3 : points.size());
    for (int i = 0; i < L.nPoints; ++i) L.points[i] = points[i];
    return L;
}

// ---------------------------------------------------------------------------
//  Inicialización
// ---------------------------------------------------------------------------
bool Renderer::init()
{
    bool ok = true;
    ok &= scene_.load("Shader/escena.vs", "Shader/escena.frag");
    ok &= crowd_.load("Shader/instancia.vs", "Shader/escena.frag");
    ok &= depth_.load("Shader/profundidad.vs", "Shader/profundidad.frag");
    ok &= sky_.load("Shader/cielo.vs", "Shader/cielo.frag");
    ok &= glow_.load("Shader/brillo.vs", "Shader/brillo.frag");
    if (!ok) return false;

    if (const char* s = std::getenv("TENIS_SOMBRA")) shadowSize_ = std::atoi(s) > 0 ? std::atoi(s) : 4096;
    GLint maxTex = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTex);
    if (shadowSize_ > maxTex) shadowSize_ = maxTex;

    glGenTextures(1, &shadowTex_);
    glBindTexture(GL_TEXTURE_2D, shadowTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, shadowSize_, shadowSize_, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float border[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    glGenFramebuffers(1, &shadowFbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex_, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR: framebuffer de sombras incompleto" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenVertexArrays(1, &dummyVao_);
    return true;
}

// ---------------------------------------------------------------------------
//  Sombras
// ---------------------------------------------------------------------------
void Renderer::beginShadow(const Lighting& L)
{
    shadowPass_ = true;
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo_);
    glViewport(0, 0, shadowSize_, shadowSize_);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 3.0f);
    depth_.use();
    depth_.setMat4("lightSpace", L.lightSpace);
}

void Renderer::endShadow(int vw, int vh)
{
    glDisable(GL_POLYGON_OFFSET_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, vw, vh);
    shadowPass_ = false;
}

// ---------------------------------------------------------------------------
//  Escena principal
// ---------------------------------------------------------------------------
void Renderer::uploadLighting(const Shader& s)
{
    s.setVec3("viewPos", eye_);
    s.setVec3("sunDir", L_.lightDir);
    s.setVec3("sunColor", L_.lightColor);
    s.setVec3("ambSky", L_.ambSky);
    s.setVec3("ambGround", L_.ambGround);
    s.setVec3("fogColor", L_.fogColor);
    s.setFloat("fogDensity", L_.fogDensity);
    s.setFloat("exposure", L_.exposure);
    s.setMat4("lightSpace", L_.lightSpace);
    s.setBool("shadowsOn", true);
    s.setFloat("shadowTexel", 1.0f / shadowSize_);
    s.setInt("nSpots", L_.nSpots);
    for (int i = 0; i < L_.nSpots; ++i)
    {
        s.setVec3At("spotPos", i, L_.spots[i].pos);
        s.setVec3At("spotDir", i, L_.spots[i].dir);
        s.setVec3At("spotColor", i, L_.spots[i].color);
        s.setVec2At("spotCone", i, glm::vec2(L_.spots[i].innerCos, L_.spots[i].outerCos));
    }
    s.setInt("nPoints", L_.nPoints);
    for (int i = 0; i < L_.nPoints; ++i)
    {
        s.setVec3At("pointPos", i, L_.points[i].pos);
        s.setVec3At("pointColor", i, L_.points[i].color);
    }
    s.setInt("texDiffuse", 0);
    s.setInt("texNormal", 1);
    s.setInt("texDecal", 2);
    s.setInt("shadowMap", 5);
}

void Renderer::beginScene(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& eye, const Lighting& L,
                          float time, int vw, int vh)
{
    view_ = view; proj_ = proj; eye_ = eye; L_ = L;
    glViewport(0, 0, vw, vh);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDisable(GL_BLEND);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, shadowTex_);
    glActiveTexture(GL_TEXTURE0);

    scene_.use();
    scene_.setMat4("view", view_);
    scene_.setMat4("projection", proj_);
    uploadLighting(scene_);

    crowd_.use();
    crowd_.setMat4("view", view_);
    crowd_.setMat4("projection", proj_);
    crowd_.setFloat("time", time);
    uploadLighting(crowd_);
    scene_.use();
    (void)time;
}

void Renderer::drawSky(const Lighting& L, float time)
{
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    sky_.use();
    sky_.setMat4("invProj", glm::inverse(proj_));
    sky_.setMat3("invViewRot", glm::transpose(glm::mat3(view_)));
    sky_.setVec3("sunDir", L.sunDir);
    sky_.setVec3("moonDir", L.moonDir);
    sky_.setFloat("dayFactor", L.dayFactor);
    sky_.setVec3("fogColor", L.fogColor);
    sky_.setFloat("exposure", L.exposure);
    sky_.setFloat("time", time);
    glBindVertexArray(dummyVao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    scene_.use();
}

void Renderer::setMaterial(const Shader& s, const Material& m)
{
    s.setVec3("matColor", m.color);
    s.setFloat("matSpec", m.specular);
    s.setFloat("matShin", m.shininess);
    s.setVec3("matEmissive", m.emissive);
    s.setFloat("selfIllum", m.selfIllum);
    s.setFloat("matAlpha", m.alpha);
    s.setVec2("uvScale", m.uvScale);
    s.setBool("alphaTest", m.alphaTest);

    bool hasTex = m.tex != 0;
    s.setBool("useTex", hasTex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m.tex);

    bool hasN = m.normalTex != 0;
    s.setBool("useNormalMap", hasN);
    s.setFloat("normalStrength", m.normalStrength);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, hasN ? m.normalTex : 0);

    bool hasD = m.decalTex != 0;
    s.setBool("useDecal", hasD);
    if (hasD)
    {
        s.setVec2("decalOrigin", glm::vec2(Textures::DECAL_X0, Textures::DECAL_Z0));
        s.setVec2("decalSize", glm::vec2(Textures::DECAL_W, Textures::DECAL_H));
        s.setVec3("decalOutsideTint", m.outsideTint);
    }
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, hasD ? m.decalTex : 0);
    glActiveTexture(GL_TEXTURE0);
}

void Renderer::draw(const Mesh& m, const glm::mat4& model, const Material& mat)
{
    if (!m.valid()) return;
    if (shadowPass_)
    {
        if (!mat.castShadow || mat.blend || mat.alphaTest || mat.alpha < 0.5f) return;
        depth_.setMat4("model", model);
        m.draw();
        return;
    }
    scene_.setMat4("model", model);
    scene_.setMat3("normalMat", glm::transpose(glm::inverse(glm::mat3(model))));
    setMaterial(scene_, mat);

    if (mat.doubleSided) glDisable(GL_CULL_FACE);
    if (mat.blend)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
    }
    m.draw();
    if (mat.blend) { glDepthMask(GL_TRUE); glDisable(GL_BLEND); }
    if (mat.doubleSided) glEnable(GL_CULL_FACE);
}

void Renderer::drawModel(const Model& mdl, const glm::mat4& model, const glm::vec3& tint)
{
    for (size_t i = 0; i < mdl.parts.size(); ++i)
    {
        const ModelPart& p = mdl.parts[i];
        Material m;
        m.color = p.color * tint;
        m.tex = p.tex;
        m.specular = p.specular * 0.5f;
        m.shininess = p.shininess;
        m.alphaTest = p.alphaTest;
        m.doubleSided = true;               // los .obj externos pueden venir con caras en cualquier sentido
        draw(p.mesh, model, m);
    }
}

void Renderer::drawCrowd(const Mesh& m, int count, float time, float cheer, float waveOn, float waveTime)
{
    if (shadowPass_ || !m.valid() || count <= 0) return;
    crowd_.use();
    crowd_.setFloat("time", time);
    crowd_.setFloat("cheer", cheer);
    crowd_.setFloat("waveOn", waveOn);
    crowd_.setFloat("waveTime", waveTime);
    crowd_.setMat4("model", glm::mat4(1.0f));
    crowd_.setMat3("normalMat", glm::mat3(1.0f));
    Material mat(glm::vec3(1.0f), 0.04f, 8.0f);
    setMaterial(crowd_, mat);
    m.drawInstanced(count);
    scene_.use();
}

void Renderer::drawGlow(const std::vector<glm::vec3>& centers, float size, const glm::vec3& color)
{
    if (shadowPass_ || centers.empty()) return;
    glow_.use();
    glow_.setMat4("view", view_);
    glow_.setMat4("projection", proj_);
    glow_.setFloat("size", size);
    glow_.setVec3("color", color);
    for (size_t i = 0; i < centers.size() && i < 16; ++i) glow_.setVec3At("centers", static_cast<int>(i), centers[i]);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glBindVertexArray(dummyVao_);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(centers.size() < 16 ? centers.size() : 16));
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    scene_.use();
}
