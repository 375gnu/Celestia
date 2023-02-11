// starfield.cpp
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <fmt/format.h>
#include "glsupport.h"
#include <celutil/color.h>
#include "objectrenderer.h"
#include "shadermanager.h"
#include "render.h"
#include "texture.h"
#include "pointstarvertexbuffer.h"

PointStarVertexBuffer::PointStarVertexBuffer(const Renderer &renderer,
                                             capacity_t capacity) :
    m_renderer(renderer),
    m_capacity(capacity),
    m_vertices(std::make_unique<StarVertex[]>(capacity)),
    m_vao(sizeof(StarVertex) * capacity, GL_STREAM_DRAW)
{
}

void PointStarVertexBuffer::start()
{
    m_prog = m_renderer.getShaderManager().getShader("star");
    if (m_prog == nullptr)
        return;
#ifndef GL_ES
    glEnable(GL_POINT_SPRITE);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif
    m_prog->use();
    m_prog->setMVPMatrices(m_renderer.getCurrentProjectionMatrix(), m_renderer.getCurrentModelViewMatrix());
    m_prog->samplerParam("starTex") = 0;
    m_prog->floatParam("limitingMagnitude") = m_limitingMagnitude;
    m_prog->floatParam("pointScale") = m_pointScale;
}

void PointStarVertexBuffer::render()
{
    if (m_nStars == 0)
        return;

    if (m_texture != nullptr)
        m_texture->bind();

    setupVertexArrayObject();
    m_vao.allocate(nullptr); // orphan the buffer
    m_vao.setBufferData(m_vertices.get(), 0, m_nStars * sizeof(StarVertex));
    m_vao.draw(GL_POINTS, m_nStars);
    m_nStars = 0;
    //fmt::print("render -> {}\n", glGetError());
}

void PointStarVertexBuffer::setupVertexArrayObject()
{
    if (m_vao.initialized())
    {
        m_vao.bindWritable();
    }
    else
    {
        m_vao.bind();

        m_vao.setVertexAttribArray(
            CelestiaGLProgram::VertexCoordAttributeIndex,
            3,
            GL_FLOAT,
            false,
            sizeof(StarVertex),
            offsetof(StarVertex, position));

        m_vao.setVertexAttribArray(
            CelestiaGLProgram::ColorAttributeIndex,
            4,
            GL_UNSIGNED_BYTE,
            true,
            sizeof(StarVertex),
            offsetof(StarVertex, color));

        m_vao.setVertexAttribArray(
            CelestiaGLProgram::PointSizeAttributeIndex,
            1,
            GL_FLOAT,
            false,
            sizeof(StarVertex),
            offsetof(StarVertex, size));
    }
    //fmt::print("setupVertexArrayObject -> {}\n", glGetError());
}

void PointStarVertexBuffer::finish()
{
    render();
    m_vao.unbind();
#ifndef GL_ES
    glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glDisable(GL_POINT_SPRITE);
#endif
}

void PointStarVertexBuffer::setTexture(Texture *texture)
{
    m_texture = texture;
}

void PointStarVertexBuffer::setPointScale(float pointSize)
{
    m_pointScale = pointSize;
}

void PointStarVertexBuffer::setLimitingMagnitude(float limitingMagnitude)
{
    m_limitingMagnitude = limitingMagnitude;
}
