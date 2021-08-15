// starfield.cpp
//
// Copyright (C) 2001-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "glsupport.h"
#include <celutil/color.h>
#include "objectrenderer.h"
#include "shadermanager.h"
#include "render.h"
#include "texture.h"
#include "pointstarvertexbuffer.h"

PointStarVertexBuffer::PointStarVertexBuffer(const Renderer& _renderer,
                                             unsigned int _capacity) :
    renderer(_renderer),
    capacity(_capacity)
{
    vertices = new StarVertex[capacity];
}

PointStarVertexBuffer::~PointStarVertexBuffer()
{
    delete[] vertices;
}

void PointStarVertexBuffer::start()
{
    program = renderer.getShaderManager().getShader("star");
    if (program == nullptr)
        return;

    program->use();
    program->setMVPMatrices(renderer.getProjectionMatrix(), renderer.getModelViewMatrix());
    program->samplerParam("starTex") = 0;
    program->floatParam("limitingMagnitude") = limitingMagnitude;
    program->floatParam("pointScale") = pointScale;
    std::array<int, 4> viewport;
    renderer.getViewport(viewport);
    program->vec4Param("viewport") = Eigen::Map<Eigen::Vector4i>(viewport.data()).cast<float>();
    glEnableVertexAttribArray(CelestiaGLProgram::PointSizeAttributeIndex);
    glEnableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glEnableVertexAttribArray(CelestiaGLProgram::ColorAttributeIndex);
}

void PointStarVertexBuffer::render()
{
    if (nStars != 0)
    {
        unsigned int stride = sizeof(StarVertex);
        glVertexAttribPointer(CelestiaGLProgram::VertexCoordAttributeIndex,
                              3, GL_FLOAT, GL_FALSE,
                              stride, &vertices[0].position);
        glVertexAttribPointer(CelestiaGLProgram::ColorAttributeIndex,
                              4, GL_UNSIGNED_BYTE, GL_TRUE,
                              stride, &vertices[0].color);
        glVertexAttribPointer(CelestiaGLProgram::PointSizeAttributeIndex,
                              1, GL_FLOAT, GL_FALSE,
                              stride, &vertices[0].size);
        if (texture != nullptr)
            texture->bind();
        glDrawArrays(GL_POINTS, 0, nStars);
        nStars = 0;
    }
}

void PointStarVertexBuffer::finish()
{
    render();
    glDisableVertexAttribArray(CelestiaGLProgram::ColorAttributeIndex);
    glDisableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glDisableVertexAttribArray(CelestiaGLProgram::PointSizeAttributeIndex);
}

void PointStarVertexBuffer::enable()
{
#ifndef GL_ES
    glEnable(GL_POINT_SPRITE);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif
}

void PointStarVertexBuffer::disable()
{
#ifndef GL_ES
    glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glDisable(GL_POINT_SPRITE);
#endif
}

void PointStarVertexBuffer::setTexture(Texture* _texture)
{
    texture = _texture;
}

void PointStarVertexBuffer::setPointScale(float pointSize)
{
    pointScale = pointSize;
}

void PointStarVertexBuffer::setLimitingMagnitude(float _limitingMagnitude)
{
    limitingMagnitude = _limitingMagnitude;
}
