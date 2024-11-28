#include "window.h"

#define NOMINMAX
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

namespace xjar {

static Window g_window;

Window &GetWindow() {
    return g_window;
}

void InitWindow(u32 width, u32 height, const char *title) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(width, height, title, nullptr, nullptr);

    g_window.width = width;
    g_window.height = height;
    g_window.title = title;
    g_window.handle = window;
    g_window.nativeHandle = glfwGetWin32Window(window);
}

}
