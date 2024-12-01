#pragma once

#include "types.h"
#include <string>

namespace xjar {

struct Window {
    u32          width;
    u32          height;
    std::string  title;
    void *       handle;
    void *       nativeHandle; // used for SDL,GLFW wrappers
    b32          resized = false;
    b32          requestExit = false;
};

Window &GetWindow();
void    InitWindow(u32 width, u32 height, const char *title);
void    SetWindowParams(u32 width, u32 height, const char *title, void *handle, void *nativeHandle);

}
