#pragma once

#include <glad/glad.h>

namespace xjar {

inline void glCheckError_(const char *file, int line) {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {

        fprintf(stderr, "glError: %d | file: %s#%d\n", err, file, line);
    }
}

#define glCheckError() glCheckError_(__FILE__, __LINE__)

}



