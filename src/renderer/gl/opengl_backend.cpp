#include "opengl_backend.h"
#include "window.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace xjar {

static const char *shaderCodeVertex = R"(
#version 460 core
layout (location=0) out vec3 color;

const vec2 pos[3] = vec2[3](
  vec2(-0.6, -0.4),
  vec2(0.6, -0.4),
  vec2(0.0, 0.6)
);

const vec3 col[3] = vec3[3](
 vec3(1.0, 0.0, 0.0),
  vec3(0.0, 1.0, 0.0),
  vec3(0.0, 0.0, 1.0)

);

void main() {

  gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);

  color = col[gl_VertexID];

}

)";

static const char *shaderCodeFragment = R"(

#version 460 core

layout (location=0) in vec3 color;
layout (location=0) out vec4 out_FragColor;

void main() {
  out_FragColor = vec4(color, 1.0);
};

)";

GLuint VAO;
GLFWwindow *g_window;

GLuint CreateShader(const char *vs, const char *fs) {
    const GLuint shaderVertex = glCreateShader(GL_VERTEX_SHADER);
    
    glShaderSource(shaderVertex, 1, &shaderCodeVertex, nullptr);
    glCompileShader(shaderVertex);

    const GLuint shaderFragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(shaderFragment, 1, &shaderCodeFragment, nullptr);
    glCompileShader(shaderFragment);

    const GLuint program = glCreateProgram();

    glAttachShader(program, shaderVertex);
    glAttachShader(program, shaderFragment);
    glLinkProgram(program);

    glDeleteShader(shaderFragment);
    glDeleteShader(shaderVertex);

    glUseProgram(program);

    return program;
}

void OpenGL_Backend::OnInit() {
    m_mainShader = CreateShader(shaderCodeVertex, shaderCodeFragment);
    g_window = static_cast<GLFWwindow *>(xjar::GetWindow().handle);
    
    glCreateVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
}
void OpenGL_Backend::OnDestroy() {
    glDeleteProgram(m_mainShader);
    glDeleteVertexArrays(1, &VAO);
}

void OpenGL_Backend::OnResized(u32 width, u32 height) {
}
void OpenGL_Backend::UpdateGlobalState(const glm::mat4 &proj, const glm::mat4 &view) {
}
void OpenGL_Backend::CreateTexture(const void *pixels, Texture *texture) {
}
void OpenGL_Backend::DestroyTexture(Texture *texture) {
}
void OpenGL_Backend::BeginFrame(f32 dt) {
    int w, h;

    glfwGetFramebufferSize(g_window, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGL_Backend::DrawGeometry() {
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
void OpenGL_Backend::EndFrame(f32 dt) {
    glfwSwapBuffers(g_window);
}

}
