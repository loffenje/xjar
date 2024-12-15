#include "opengl_backend.h"
#include "opengl_mesh.h"
#include "window.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "opengl_shared.h"

namespace xjar {


GLFWwindow *g_window;

void DebugMessageCallback(GLenum source, GLenum type, GLuint id,
    GLenum severity, GLsizei length,
    const GLchar *message, const void *param) {
    // Convert GLenum parameters to strings

    auto typeStr = std::to_string(type);
    auto sourceStr = std::to_string(source);
    auto severityStr = std::to_string(severity);

    fprintf(stderr, "%s:%s[%s](%d): %sn\n", sourceStr.c_str(), typeStr.c_str(),
           severityStr.c_str(), id, message);
}

void OpenGL_Backend::OnInit() {
    glEnable(GL_DEBUG_OUTPUT);

    glDebugMessageCallback(DebugMessageCallback, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);

    g_window = static_cast<GLFWwindow *>(xjar::GetWindow().handle);

}
void OpenGL_Backend::OnDestroy() {
}

void OpenGL_Backend::OnResized(u32 width, u32 height) {
}
void OpenGL_Backend::UpdateGlobalState(const GPU_SceneData &sceneData) {
}

void OpenGL_Backend::CreateTexture(const void *pixels, Texture *texture) {
    GLuint *gltexture = (GLuint *)malloc(sizeof(GLuint));
    glCreateTextures(GL_TEXTURE_2D, 1, gltexture);
    glCheckError();

    texture->handle = gltexture;
    glTextureParameteri(*gltexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(*gltexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTextureStorage2D(*gltexture, 1, GL_RGB8, texture->width, texture->height);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTextureSubImage2D(*gltexture, 0, 0, 0, texture->width, texture->height,
                        GL_RGBA, GL_UNSIGNED_BYTE, pixels);
 
    glBindTextureUnit(0, *gltexture);
}

void OpenGL_Backend::CreateMesh(Model &model) {
    size_t vertexDataSize = model.mesh.vertexData.size() * sizeof(model.mesh.vertexData[0]);
    size_t indexDataSize = model.mesh.indexData.size() * sizeof(model.mesh.indexData[0]);

    OpenGL_Mesh *glmesh = new OpenGL_Mesh();
    glmesh->Init(model.mesh.indexData.data(), indexDataSize, 
            model.mesh.vertexData.data(), vertexDataSize);

    model.handle = glmesh;
}

void OpenGL_Backend::DestroyTexture(Texture *texture) {
}

void OpenGL_Backend::BeginDefaultPass() {
    glEnable(GL_DEPTH_TEST);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}

void OpenGL_Backend::EndDefaultPass() {
    glDisable(GL_DEPTH_TEST);
}

FrameStatus OpenGL_Backend::BeginFrame() {
    int w, h;

    glfwGetFramebufferSize(g_window, &w, &h);
    glViewport(0, 0, w, h);

    FrameStatus frame{.success = true, .data = nullptr};
    return frame;
}

void OpenGL_Backend::EndFrame() {
    glfwSwapBuffers(g_window);
}

}
