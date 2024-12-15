#pragma once

#include <glad/glad.h>
#include "renderer/renderer_types.h"

namespace xjar {

class OpenGL_Mesh {
public:
    OpenGL_Mesh() = default;
    ~OpenGL_Mesh();

    void Init(const u32 *indices, u32 indicesSizeInBytes, const f32 *vertices, u32 verticesSizeInBytes);
    void Draw();
private:
    GLuint m_vertexBuffer;
    GLuint m_indexBuffer;
    GLuint m_vertexArray;
    u32    m_numIndices;
};

}
