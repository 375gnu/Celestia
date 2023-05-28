// binder.cpp
//
// Copyright (C) 2023-present, Celestia Development Team.
//
// VAO wrapper.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
#include "binder.h"

namespace celestia::gl
{

Binder&
Binder::get()
{
    static Binder *binder = nullptr;
    if (binder == nullptr)
        binder = new Binder();
    return *binder;
}

Binder&
Binder::bind(const Buffer &bo)
{
    return bindVBO(GLenum(bo.targetHint()), bo.id());
}

Binder&
Binder::unbind(const Buffer &bo)
{
    return bindVBO(GLenum(bo.targetHint()), 0u);
}

Binder&
Binder::bind(const VertexObject &vo)
{
    return bindVAO(vo.id());
}

Binder&
Binder::unbind(const VertexObject & /*vo*/)
{
    return bindVAO(0u);
}

Binder&
Binder::bindVAO(GLuint id)
{
    if (m_boundVao != id)
    {
        glBindVertexArray(id);
        m_boundVao = id;
        m_boundVbo = 0u;
        m_boundIbo = 0u;
    }
    return *this;
}

Binder&
Binder::bindVBO(GLuint target, GLuint id)
{
    glBindBuffer(target, id);
    switch (target)
    {
    case GL_ARRAY_BUFFER:
        if (m_boundVbo != id)
        {
            m_boundVbo = id;
            glBindBuffer(target, id);
        }
        break;
    case GL_ELEMENT_ARRAY_BUFFER:
        if (m_boundIbo != id)
        {
            m_boundIbo = id;
            glBindBuffer(target, id);
        }
        break;
    default:
        break;
    }
    return *this;
}

}
