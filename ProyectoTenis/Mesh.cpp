#include "Mesh.h"
#include <cstddef>

// ---------------------------------------------------------------- Mesh
void Mesh::upload(const std::vector<Vertex>& vv, const std::vector<unsigned int>& ii)
{
    count = static_cast<GLsizei>(ii.size());
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vv.size() * sizeof(Vertex), vv.empty() ? NULL : &vv[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ii.size() * sizeof(unsigned int), ii.empty() ? NULL : &ii[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, nrm));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, col));
    glBindVertexArray(0);
}

void Mesh::setInstances(const std::vector<float>& data)
{
    if (!instVbo) glGenBuffers(1, &instVbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, instVbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.empty() ? NULL : &data[0], GL_STATIC_DRAW);
    const GLsizei stride = 8 * sizeof(float);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);
    glVertexAttribDivisor(4, 1);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(4 * sizeof(float)));
    glVertexAttribDivisor(5, 1);
    glBindVertexArray(0);
}

void Mesh::draw() const
{
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Mesh::drawInstanced(int n) const
{
    glBindVertexArray(vao);
    glDrawElementsInstanced(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0, n);
    glBindVertexArray(0);
}

// ---------------------------------------------------------------- MeshBuilder
unsigned int MeshBuilder::addV(const M4& xf, const V3& p, const V3& n, const V2& uv, const V4& col)
{
    Vertex vt;
    vt.pos = V3(xf * V4(p, 1.0f));
    vt.nrm = glm::normalize(glm::mat3(xf) * n);
    vt.uv = uv;
    vt.col = col;
    v.push_back(vt);
    return static_cast<unsigned int>(v.size() - 1);
}

// Agrega un triángulo corrigiendo el sentido (CCW visto desde afuera según la normal del vértice)
void MeshBuilder::tri(unsigned int a, unsigned int b, unsigned int c)
{
    V3 e1 = v[b].pos - v[a].pos, e2 = v[c].pos - v[a].pos;
    V3 fn = glm::cross(e1, e2);
    V3 nn = v[a].nrm + v[b].nrm + v[c].nrm;
    if (glm::dot(fn, nn) < 0.0f) { unsigned int t = b; b = c; c = t; }
    idx.push_back(a); idx.push_back(b); idx.push_back(c);
}

void MeshBuilder::box(const M4& xf, const V3& size, const V2& tile, const V4& col)
{
    static const V3 N[6] = { V3(1,0,0), V3(-1,0,0), V3(0,1,0), V3(0,-1,0), V3(0,0,1), V3(0,0,-1) };
    static const V3 Uax[6] = { V3(0,0,-1), V3(0,0,1), V3(1,0,0), V3(1,0,0), V3(1,0,0), V3(-1,0,0) };
    static const V3 Vax[6] = { V3(0,1,0), V3(0,1,0), V3(0,0,-1), V3(0,0,1), V3(0,1,0), V3(0,1,0) };
    V3 h = size * 0.5f;
    for (int f = 0; f < 6; ++f)
    {
        float hn = glm::dot(glm::abs(N[f]), h);
        float hu = glm::dot(glm::abs(Uax[f]), h);
        float hv = glm::dot(glm::abs(Vax[f]), h);
        V3 c = N[f] * hn;
        V3 p00 = c - Uax[f] * hu - Vax[f] * hv;
        V3 p10 = c + Uax[f] * hu - Vax[f] * hv;
        V3 p11 = c + Uax[f] * hu + Vax[f] * hv;
        V3 p01 = c - Uax[f] * hu + Vax[f] * hv;
        float su = 2.0f * hu / tile.x, sv = 2.0f * hv / tile.y;
        // v=0 arriba: el borde superior de la cara (p01,p11) usa v=0
        unsigned int a = addV(xf, p00, N[f], V2(0.0f, sv), col);
        unsigned int b = addV(xf, p10, N[f], V2(su, sv), col);
        unsigned int cc = addV(xf, p11, N[f], V2(su, 0.0f), col);
        unsigned int d = addV(xf, p01, N[f], V2(0.0f, 0.0f), col);
        tri(a, b, cc);
        tri(a, cc, d);
    }
}

void MeshBuilder::boxAt(const V3& mn, const V3& mx, const V2& tile, const V4& col)
{
    box(glm::translate(M4(1.0f), (mn + mx) * 0.5f), mx - mn, tile, col);
}

void MeshBuilder::plane(const M4& xf, float sx, float sz, const V2& tile, const V4& col)
{
    float hx = sx * 0.5f, hz = sz * 0.5f;
    V3 n(0, 1, 0);
    unsigned int a = addV(xf, V3(-hx, 0, -hz), n, V2(-hx / tile.x, -hz / tile.y), col);
    unsigned int b = addV(xf, V3(hx, 0, -hz), n, V2(hx / tile.x, -hz / tile.y), col);
    unsigned int c = addV(xf, V3(hx, 0, hz), n, V2(hx / tile.x, hz / tile.y), col);
    unsigned int d = addV(xf, V3(-hx, 0, hz), n, V2(-hx / tile.x, hz / tile.y), col);
    tri(a, c, b);
    tri(a, d, c);
}

void MeshBuilder::quad(const M4& xf, float w, float h, const V2& uvMax, const V4& col, bool doubleSide)
{
    float hx = w * 0.5f, hy = h * 0.5f;
    for (int side = 0; side < (doubleSide ? 2 : 1); ++side)
    {
        float s = side == 0 ? 1.0f : -1.0f;
        V3 n(0, 0, s);
        unsigned int a = addV(xf, V3(-hx * s, -hy, 0), n, V2(0.0f, uvMax.y), col);
        unsigned int b = addV(xf, V3(hx * s, -hy, 0), n, V2(uvMax.x, uvMax.y), col);
        unsigned int c = addV(xf, V3(hx * s, hy, 0), n, V2(uvMax.x, 0.0f), col);
        unsigned int d = addV(xf, V3(-hx * s, hy, 0), n, V2(0.0f, 0.0f), col);
        tri(a, b, c);
        tri(a, c, d);
    }
}

void MeshBuilder::cylinder(const M4& xf, float rB, float rT, float h, int segs, bool caps, const V2& tile, const V4& col)
{
    float slope = (rB - rT) / h;
    std::vector<unsigned int> bot, top;
    float circ = 2.0f * U::PI * (rB > rT ? rB : rT);
    for (int i = 0; i <= segs; ++i)
    {
        float a = 2.0f * U::PI * i / segs;
        float c = std::cos(a), s = std::sin(a);
        V3 n = glm::normalize(V3(c, slope, s));
        float u = (circ * i / segs) / tile.x;
        bot.push_back(addV(xf, V3(rB * c, 0.0f, rB * s), n, V2(u, h / tile.y), col));
        top.push_back(addV(xf, V3(rT * c, h, rT * s), n, V2(u, 0.0f), col));
    }
    for (int i = 0; i < segs; ++i)
    {
        tri(bot[i], bot[i + 1], top[i + 1]);
        tri(bot[i], top[i + 1], top[i]);
    }
    if (!caps) return;
    for (int cap = 0; cap < 2; ++cap)
    {
        float r = cap == 0 ? rB : rT;
        if (r <= 0.0001f) continue;
        float y = cap == 0 ? 0.0f : h;
        V3 n(0, cap == 0 ? -1.0f : 1.0f, 0);
        unsigned int center = addV(xf, V3(0, y, 0), n, V2(0.5f, 0.5f), col);
        std::vector<unsigned int> ring;
        for (int i = 0; i <= segs; ++i)
        {
            float a = 2.0f * U::PI * i / segs;
            ring.push_back(addV(xf, V3(r * std::cos(a), y, r * std::sin(a)), n,
                                V2(0.5f + 0.5f * std::cos(a), 0.5f + 0.5f * std::sin(a)), col));
        }
        for (int i = 0; i < segs; ++i) tri(center, ring[i], ring[i + 1]);
    }
}

void MeshBuilder::sphere(const M4& xf, float r, int segs, int rings, const V4& col)
{
    std::vector<std::vector<unsigned int> > grid;
    for (int j = 0; j <= rings; ++j)
    {
        float th = U::PI * j / rings;
        std::vector<unsigned int> row;
        for (int i = 0; i <= segs; ++i)
        {
            float ph = 2.0f * U::PI * i / segs;
            V3 n(std::sin(th) * std::cos(ph), std::cos(th), std::sin(th) * std::sin(ph));
            row.push_back(addV(xf, n * r, n, V2(static_cast<float>(i) / segs, static_cast<float>(j) / rings), col));
        }
        grid.push_back(row);
    }
    for (int j = 0; j < rings; ++j)
        for (int i = 0; i < segs; ++i)
        {
            unsigned int a = grid[j][i], b = grid[j][i + 1], c = grid[j + 1][i + 1], d = grid[j + 1][i];
            if (j != 0) tri(a, d, b);
            if (j != rings - 1) tri(b, d, c);
        }
}

void MeshBuilder::bar(const V3& p0, const V3& p1, float r, int segs, const V4& col)
{
    V3 d = p1 - p0;
    float len = glm::length(d);
    if (len < 1e-5f) return;
    d /= len;
    V3 y(0, 1, 0);
    M4 xf = glm::translate(M4(1.0f), p0);
    float c = glm::dot(y, d);
    if (c < -0.9999f) xf = xf * glm::rotate(M4(1.0f), U::PI, V3(1, 0, 0));
    else if (c < 0.9999f)
    {
        V3 axis = glm::normalize(glm::cross(y, d));
        xf = xf * glm::rotate(M4(1.0f), std::acos(c), axis);
    }
    cylinder(xf, r, r, len, segs, true, V2(1.0f), col);
}

void MeshBuilder::append(const MeshBuilder& o, const M4& xf)
{
    unsigned int base = static_cast<unsigned int>(v.size());
    for (size_t i = 0; i < o.v.size(); ++i)
        addV(xf, o.v[i].pos, o.v[i].nrm, o.v[i].uv, o.v[i].col);
    for (size_t i = 0; i < o.idx.size(); ++i) idx.push_back(base + o.idx[i]);
}

Mesh MeshBuilder::build() const
{
    Mesh m;
    m.upload(v, idx);
    return m;
}
