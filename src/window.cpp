#include "window.h"

namespace xjar {

static Window g_window;

Window &GetWindow() {
    return g_window;
}

void SetWindowParams(u32 width, u32 height, const char *title, void *handle, void *nativeHandle) {
    g_window.width = width;
    g_window.height = height;
    g_window.title = title;
    g_window.handle = handle;
    g_window.nativeHandle = nativeHandle;
}

}
