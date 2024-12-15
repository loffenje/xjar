#include <glad/glad.h>
#include "opengl_pipeline.h"

namespace xjar {

void OpenGL_Shader::CreateShader(const char *vs, const char *fs) {
    const GLuint shaderVertex = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(shaderVertex, 1, &vs, nullptr);
    glCompileShader(shaderVertex);
    CheckErrors(shaderVertex, "VERTEX");

    const GLuint shaderFragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(shaderFragment, 1, &fs, nullptr);
    glCompileShader(shaderFragment);
    CheckErrors(shaderFragment, "FRAGMENT");

    program = glCreateProgram();

    glAttachShader(program, shaderVertex);
    glAttachShader(program, shaderFragment);
    glLinkProgram(program);
    CheckErrors(program, "PROGRAM");

    glDeleteShader(shaderFragment);
    glDeleteShader(shaderVertex);
    glUseProgram(program);
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        fprintf(stderr, "OpenGL error: %d\n", err);
    }
}

void OpenGL_Shader::CheckErrors(GLuint shader, std::string type) {
    GLint  success;
    GLchar infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            fprintf(stderr, "ERROR::SHADER_COMPILATION_ERROR of type: %s %s\n", type.c_str(), infoLog);
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            fprintf(stderr, "ERROR::PROGRAM_LINKING_ERROR of type: %s %s\n", type.c_str(), infoLog);
        }
    }
}
void OpenGL_Shader::Bind() {
    glUseProgram(program);
}

void OpenGL_Shader::Unbind() {
    glUseProgram(0);
}


}
