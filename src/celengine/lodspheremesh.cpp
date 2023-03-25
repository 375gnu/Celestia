// lodspheremesh.cpp
//
// Copyright (C) 2000-2009, theCelestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "lodspheremesh.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>

#include <fmt/format.h>

#include <celcompat/numbers.h>
#include <celengine/render.h>
#include <celengine/shadermanager.h>
#include <celengine/texture.h>
#include <celmath/frustum.h>
#include <celmath/mathlib.h>
#include <celutil/array_view.h>
#include <celutil/arrayvector.h>

using celestia::numbers::pi;
using celmath::sincos;
using celmath::Frustum;

namespace
{

constexpr int N = 36;
constexpr int kMaxLevel = 4;

struct MappedVtx
{
    Eigen::Vector4f pos;
    Eigen::Vector2f texCoord;
};

Eigen::Vector4f
MapToSphere(double x, double y)
{
    double theta = (x - 0.5) * pi; // longitude
    double phi   = (0.5 - y) * pi; // latitude

    double sphi, cphi, stheta, ctheta;
    sincos(phi, sphi, cphi);
    sincos(theta, stheta, ctheta);

    return Eigen::Vector4f(
        static_cast<float>(cphi * ctheta),
        static_cast<float>(sphi),
        static_cast<float>(cphi * stheta),
        1.0f);
}

} // namespace

class Node;
class Patch;

/* QuadTree
*/

class QuadTree
{
public:
    QuadTree(double startX, double startY, double endX, double endY);

    void setMVPMatrix(const Eigen::Matrix4f&);
    void setModelViewMatrix(const Eigen::Matrix4f& m) { m_modelViewMatrix = m; }
    void setFrustum(const Frustum&);
    const Eigen::Matrix4f& getMVPMatrix() const { return m_mvp; }
    const Frustum* getFrustum() const { return m_frustum; }
    void setCameraPosition(const Eigen::Vector3f& p) { m_cameraPosition = p; }
    const Eigen::Vector3f& getCameraPosition() const { return m_cameraPosition; }
    const Eigen::Vector3f& getCameraDirection() const { return m_cameraDirection; }
    Eigen::Matrix3f getCameraMatrix() const { return m_modelViewMatrix.topLeftCorner(3,3); }
    void setViewport(int w, int h);
    const Eigen::Vector2i getViewport() const { return m_viewport; }

    void render() const;

private:
    const Patch* getPatch(int level) const;
    const Patch* createPatch(int level, double width, double height);

    Eigen::Matrix4f m_mvp;
    Eigen::Matrix4f m_modelViewMatrix;
    Eigen::Vector3f m_cameraPosition;
    Eigen::Vector3f m_cameraDirection{ 0.0f, 0.0f, 1.0f };
    Eigen::Vector2i m_viewport;

    const Frustum *m_frustum{ nullptr };

    std::vector<std::unique_ptr<Patch>> m_patches;

    std::unique_ptr<Node> m_nodeFront;
    std::unique_ptr<Node> m_nodeRear;

    friend class Node;
};

/* Node
*/

class Node
{
public:
    Node(QuadTree *qt, int level, double startX, double startY, double endX, double endY);
    ~Node() = default;

    const Patch* getPatch() const;
    const Patch* createPatch();

    bool visible() const;
    void render();

private:
    void split();
    float checkDirection() const;

    std::pair<int,int> checkVisibility(const Eigen::Matrix4f &mvp, const Eigen::Vector4f &point) const;

    std::array<std::unique_ptr<Node>, 4> m_nodes;

    QuadTree *m_qt;

    double m_startX;
    double m_startY;
    double m_endX;
    double m_endY;

    std::array<Eigen::Vector4f, 4> m_mapped;

    int m_level;
};

/* Patch
*/

class Patch
{
public:
    Patch(double width, double height);
    ~Patch() = default;

    void render(double offsetX, double offsetY) const;

private:
    struct Vertex
    {
        float pos[2];
        float texCoord[2];
    };

    struct SinCos
    {
        double s, c;
    };

    double m_width;
    double m_height;

    GLuint m_vbo { 0 };
    static GLuint m_ibo;
    static int m_idxCount;

    std::vector<SinCos> sincosX;
    std::vector<SinCos> sincosY;

    mutable std::vector<MappedVtx> mapped;

public:
    static int rendered;
};

/* QuadTree
*/

QuadTree::QuadTree(double startX, double startY, double endX, double endY) :
    m_nodeFront(std::make_unique<Node>(this, 0, startX, startY, endX/2.0, endY)),
    m_nodeRear(std::make_unique<Node>(this, 0, endX/2.0, startY, endX, endY))
{
}

const Patch*
QuadTree::getPatch(int level) const
{
    if (m_patches.size() < level + 1)
        return nullptr;

    return m_patches[level].get();
}

const Patch*
QuadTree::createPatch(int level, double width, double height)
{
    if (m_patches.size() < level + 1)
    {
        while (m_patches.size() < level)
            m_patches.emplace_back(nullptr);
        m_patches.emplace_back(std::make_unique<Patch>(width, height));
    }

    if (auto *patch = m_patches[level].get(); patch != nullptr)
        return patch;

    m_patches[level] = std::make_unique<Patch>(width, height);
    return m_patches[level].get();
}

void
QuadTree::render() const
{
    m_nodeFront->render();
    m_nodeRear->render();
}

void
QuadTree::setMVPMatrix(const Eigen::Matrix4f &mvp)
{
    m_mvp = mvp;
}

void
QuadTree::setFrustum(const Frustum &frustum)
{
    m_frustum = &frustum;
}

void
QuadTree::setViewport(int w, int h)
{
    m_viewport.x() = w;
    m_viewport.y() = h;
}

Node::Node(QuadTree *qt, int level, double startX, double startY, double endX, double endY) :
    m_qt(qt),
    m_level(level),
    m_startX(startX),
    m_startY(startY),
    m_endX(endX),
    m_endY(endY)
{
    //fmt::print("{} {}\n", m_startX,m_endX);
    m_mapped[0] = MapToSphere(m_startX, m_startY);
    m_mapped[1] = MapToSphere(m_startX, m_endY);
    m_mapped[2] = MapToSphere(m_endX, m_startY);
    m_mapped[3] = MapToSphere(m_endX, m_endY);
}

void
Node::render()
{
    if (!visible())
        return;

    bool draw = (kMaxLevel == m_level);

    if (!draw)
    {
        const Eigen::Matrix4f &mvp = m_qt->getMVPMatrix();
        auto [i0, i1] = m_startY >= 0.5f ? std::pair(3, 2) : std::pair(0, 1);

        Eigen::Vector4f v0 = mvp * m_mapped[i0];
        Eigen::Vector4f v1 = mvp * m_mapped[i1];
        auto viewport = m_qt->getViewport();
        Eigen::Vector2f s0(v0.x() * viewport.x(), v0.y() * viewport.y());
        s0 *= 1.0f / v0.w();
        Eigen::Vector2f s1(v1.x() * viewport.x(), v1.y() * viewport.y());
        s1 *= 1.0f / v1.w();
        int screensize = static_cast<int>((s0 - s1).norm());

        draw = screensize <= 38*N; // 32 + 20%
    }

    if (draw)
    {
        auto patch = getPatch();
        if (patch == nullptr)
            patch = createPatch();
        patch->render(m_startX, m_startY);
        return;
    }

    split();
    std::for_each(m_nodes.begin(), m_nodes.end(), [](const auto &node) { node->render(); });
}

void
Node::split()
{
    // endY     +---------+---------+
    //          |    2    |    3    |
    // halfY    +---------+---------+
    //          |    0    |    1    |
    // startY   +---------+---------+
    //        startX    halfX      endX
    double halfX = (m_startX + m_endX) * 0.5;
    double halfY = (m_startY + m_endY) * 0.5;
    int    level = m_level + 1;

    m_nodes[0] = std::make_unique<Node>(m_qt, level, m_startX, m_startY, halfX, halfY);
    m_nodes[1] = std::make_unique<Node>(m_qt, level, halfX, m_startY, m_endX, halfY);
    m_nodes[2] = std::make_unique<Node>(m_qt, level, m_startX, halfY, halfX, m_endY);
    m_nodes[3] = std::make_unique<Node>(m_qt, level, halfX, halfY, m_endX, m_endY);
}

const Patch*
Node::getPatch() const
{
    return m_qt->getPatch(m_level);
}

const Patch*
Node::createPatch()
{
    return m_qt->createPatch(m_level, m_endX - m_startX, m_endY - m_startY);
}

std::pair<int,int>
Node::checkVisibility(const Eigen::Matrix4f &mvp, const Eigen::Vector4f &point) const
{
    Eigen::Vector4f pos = mvp * point;
    float w = 1.1f * pos.w();
    fmt::print("[{}, {}, {}, {}]\n", pos.x(), pos.y(), pos.z(), pos.w());
    int x = pos.x() < -w ? -1 : pos.x() > w ? 1 : 0;
    int y = pos.y() < -w ? -1 : pos.y() > w ? 1 : 0;
    return {x, y};
}

float
Node::checkDirection() const
{
    return -1.0f;
    Eigen::Vector3f line1((m_mapped[0] - m_mapped[3]).head(3));
    Eigen::Vector3f line2((m_mapped[1] - m_mapped[2]).head(3));
    Eigen::Vector3f normal(line1.cross(line2).normalized());

    return (m_qt->getCameraMatrix() * normal).dot(m_qt->getCameraDirection());
}

bool
Node::visible() const
{
    if (m_level == 0)
        return true;

    const Eigen::Matrix4f &mvp = m_qt->getMVPMatrix();

    std::array<std::pair<int,int>, 4> positions;
    int x = 0, y =0;
    for (int i = 0; i < 4; i++)
    {
        positions[i] = checkVisibility(mvp, m_mapped[i]);
        if (positions[i] == std::pair(0, 0))
            return true;
        x += positions[i].first;
        y += positions[i].second;
    }

    if (!(x > -4 && x < 4) && (y > -4 && y < 4))
        return false;

    return checkDirection() < 0.2f;
}

int Patch::rendered = 0;
GLuint Patch::m_ibo = 0;
int Patch::m_idxCount = 0;

Patch::Patch(double width, double height) :
    m_width(width),
    m_height(height),
    sincosX(N+1),
    sincosY(N+1)
{
    mapped.reserve((N+1)*(N+1));

    const double stepX = m_width / static_cast<double>(N);
    const double stepY = m_height / static_cast<double>(N);

    double x = 0, y = 0;
    for (int i = 0; i <= N; i++)
    {
        sincos(x * pi, sincosX[i].s, sincosX[i].c);
        sincos(-y * pi, sincosY[i].s, sincosY[i].c);
        x += stepX;
        y += stepY;
    }

    glGenBuffers(1, &m_vbo);

    if (m_ibo == 0)
    {
        std::vector<unsigned short> indices;
        indices.reserve(2*N*(N+1) + 2*N - 2);
        for (int i = 0; i < N; i++)
        {
            int baseVertex = i * (N+1);

            if (i > 0)
                indices.push_back(baseVertex);
            for (int j = 0; j <= N; j++)
            {
                int v = baseVertex + j;
                indices.push_back(v);
                indices.push_back(v + N + 1);
            }
            if (i < N-1)
                indices.push_back(indices.back());
        }
        glGenBuffers(1, &m_ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
        m_idxCount = indices.size();
    }
}

void
Patch::render(double offsetX, double offsetY) const
{
    const double stepX = m_width / static_cast<double>(N);
    const double stepY = m_height / static_cast<double>(N);

    /*
     * sin(a + b) = sin(a)cos(b) + cos(a)sin(b)
     * cos(a + b) = cos(a)cos(b) - sin(a)sin(b)
     */

    /*
     * double theta = (x - 0.5) * double(PI); // longitude
     * x = offsetX + x' => theta x' * PI + (offsetX-0.5) * PI
     * double phi   = (0.5 - y) * double(PI); // latitude
     * y = offsetY + y' => phi = (0.5-offsetY) * PI + (-y') * PI
     */

    SinCos offsetXsc; sincos(offsetX * pi, &offsetXsc.s, &offsetXsc.c);
    SinCos offsetYsc; sincos((0.5 - offsetY) * pi, &offsetYsc.s, &offsetYsc.c);

    std::vector<SinCos> sincosXX(N+1);
    sincosXX.reserve(N+1);
    for (int i = 0; i <= N; i++)
    {
        sincosXX[i].s = offsetXsc.s * sincosX[i].c + offsetXsc.c * sincosX[i].s;
        sincosXX[i].c = offsetXsc.c * sincosX[i].c - offsetXsc.s * sincosX[i].s;
    }

    for (int j = 0; j <= N; j++)
    {
        double sphi = offsetYsc.s * sincosY[j].c + offsetYsc.c * sincosY[j].s;
        double cphi = offsetYsc.c * sincosY[j].c - offsetYsc.s * sincosY[j].s;

        for (int i = 0; i <= N; i++)
        {
            double stheta = sincosXX[i].s;
            double ctheta = sincosXX[i].c;

            Eigen::Vector4f pos(
                static_cast<float>(cphi * ctheta),
                static_cast<float>(sphi),
                static_cast<float>(cphi * stheta),
                1.0f);
            Eigen::Vector2f texCoord(1.0f - (i * stepX + offsetX) * 0.5f, j * stepY + offsetY);
            mapped.push_back({pos, texCoord});
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, mapped.size()*sizeof(mapped[0]), nullptr, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, mapped.size()*sizeof(mapped[0]), mapped.data());

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);

    constexpr int stride = sizeof(MappedVtx);

    glEnableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glVertexAttribPointer(CelestiaGLProgram::VertexCoordAttributeIndex,
                          4, GL_FLOAT, GL_FALSE,
                          stride, (void*)offsetof(MappedVtx, pos));

    glEnableVertexAttribArray(CelestiaGLProgram::NormalAttributeIndex);
    glVertexAttribPointer(CelestiaGLProgram::NormalAttributeIndex,
                          3, GL_FLOAT, GL_FALSE,
                          stride, (void*)offsetof(MappedVtx, pos));

    glEnableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex);
    glVertexAttribPointer(CelestiaGLProgram::TextureCoord0AttributeIndex,
                          2, GL_FLOAT, GL_FALSE,
                          stride, (void*) offsetof(MappedVtx, texCoord));

    glDrawElements(GL_TRIANGLE_STRIP, m_idxCount, GL_UNSIGNED_SHORT, nullptr);

    glDisableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glDisableVertexAttribArray(CelestiaGLProgram::NormalAttributeIndex);
    glDisableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex);

    rendered++;
    mapped.clear();
}

struct TextureCoords
{
    explicit TextureCoords(int _nTexturesUsed) : nTexturesUsed(_nTexturesUsed) {}

    TextureCoords(const TextureCoords&) = delete;
    TextureCoords& operator=(const TextureCoords&) = delete;
    TextureCoords(TextureCoords&&) = delete;
    TextureCoords& operator=(TextureCoords&&) = delete;

    int nTexturesUsed;
    std::array<float, LODSphereMesh::MAX_SPHERE_MESH_TEXTURES> du{};
    std::array<float, LODSphereMesh::MAX_SPHERE_MESH_TEXTURES> dv{};
    std::array<float, LODSphereMesh::MAX_SPHERE_MESH_TEXTURES> u0{};
    std::array<float, LODSphereMesh::MAX_SPHERE_MESH_TEXTURES> v0{};
};


LODSphereMesh::~LODSphereMesh()
{
    delete m_qt;
}


void
LODSphereMesh::render(const Renderer *renderer,
                      const celmath::Frustum& frustum,
                      float pixWidth,
                      Texture** tex,
                      int nTextures)
{
    render(renderer, Normals, frustum, pixWidth, tex, nTextures);
}


void
LODSphereMesh::render(const Renderer *renderer,
                      unsigned int attributes,
                      const celmath::Frustum& frustum,
                      float pixWidth,
                      Texture* tex0,
                      Texture* tex1,
                      Texture* tex2,
                      Texture* tex3)
{
    celestia::util::ArrayVector<Texture*, 4> tex;
    if (tex0 != nullptr)
        tex.try_push_back(tex0);
    if (tex1 != nullptr)
        tex.try_push_back(tex1);
    if (tex2 != nullptr)
        tex.try_push_back(tex2);
    if (tex3 != nullptr)
        tex.try_push_back(tex3);
    render(renderer, attributes, frustum, pixWidth,
           tex.data(), static_cast<int>(tex.size()));
}


void LODSphereMesh::render(const Renderer *renderer,
                           unsigned int attributes,
                           const celmath::Frustum& frustum,
                           float pixWidth,
                           Texture** tex,
                           int nTextures)
{
    if (m_qt == nullptr)
        m_qt = new QuadTree(0.0, 0.0, 2.0, 1.0);

    for (int i = 0; i < nTextures; i++)
    {
        glActiveTexture(GL_TEXTURE0+i);
        tex[i]->bind();
    }

    Patch::rendered = 0;
    m_qt->setModelViewMatrix(renderer->getCurrentModelViewMatrix());
    m_qt->setMVPMatrix(renderer->getCurrentProjectionMatrix() * renderer->getCurrentModelViewMatrix());
    int w, h; // NOSONAR
    renderer->getViewport(nullptr, nullptr, &w, &h);
    m_qt->setViewport(w, h);

    glFrontFace(GL_CW);
    m_qt->render();
    glFrontFace(GL_CCW);

    fmt::print("Rendered {} patches\n", Patch::rendered);
}
