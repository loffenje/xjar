#include "opengl_mesh.h"
#include "opengl_shared.h"

namespace xjar {

void OpenGL_Mesh::Init(const u32 *indices, u32 indicesSizeInBytes, const f32 *vertices, u32 verticesSizeInBytes) {
    m_numIndices = indicesSizeInBytes / sizeof(u32);

    glCreateVertexArrays(1, &m_vertexArray); 
    glCreateBuffers(1, &m_vertexBuffer);
    glCreateBuffers(1, &m_indexBuffer);

    glNamedBufferStorage(m_vertexBuffer, verticesSizeInBytes, vertices, 0);
    glNamedBufferStorage(m_indexBuffer, indicesSizeInBytes, indices, 0);
    
    glVertexArrayElementBuffer(m_vertexArray, m_indexBuffer);

    constexpr u32 stride = sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(glm::vec3);

    glVertexArrayVertexBuffer(m_vertexArray, 0, m_vertexBuffer, 0, stride); // bind a buffer to the binding index point 0

    glEnableVertexArrayAttrib(m_vertexArray, 0);
    glVertexArrayAttribFormat(m_vertexArray, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(m_vertexArray, 0, 0);
    
    glEnableVertexArrayAttrib(m_vertexArray, 1);
    glVertexArrayAttribFormat(m_vertexArray, 1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec3));
    glVertexArrayAttribBinding(m_vertexArray, 1, 0);

    glEnableVertexArrayAttrib(m_vertexArray, 2);
    glVertexArrayAttribFormat(m_vertexArray, 2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3) + sizeof(glm::vec2));
    glVertexArrayAttribBinding(m_vertexArray, 2, 0);
}

void OpenGL_Mesh::Draw() {
    glBindVertexArray(m_vertexArray);
    glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, nullptr);
}

OpenGL_Mesh::~OpenGL_Mesh() {
    glDeleteVertexArrays(1, &m_vertexArray);
}

}

