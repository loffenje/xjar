#include "opengl_backend.h"

namespace xjar {

void OpenGL_Backend::OnInit() {
}
void OpenGL_Backend::OnDestroy() {
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
}
void OpenGL_Backend::DrawGeometry() {
}
void OpenGL_Backend::EndFrame(f32 dt) {
}
}
