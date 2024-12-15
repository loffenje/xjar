#pragma once

#include <glad/glad.h>
#include "renderer/renderer_backend.h"

namespace xjar {

struct OpenGL_Shader {
    void CreateShader(const char *vs, const char *fs);
    void CheckErrors(GLuint shader, std::string type);
    void Bind();
    void Unbind();

    GLuint program;
};

}
