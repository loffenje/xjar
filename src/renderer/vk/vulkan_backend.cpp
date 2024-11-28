#include "vulkan_backend.h"
#include <glm/mat4x4.hpp>

namespace xjar {

void Vulkan_Backend::OnInit() {
}
void Vulkan_Backend::OnDestroy() {
}
void Vulkan_Backend::OnResized(u32 width, u32 height) {
}
void Vulkan_Backend::UpdateGlobalState(const glm::mat4 &proj, const glm::mat4 &view) {
}
void Vulkan_Backend::CreateTexture(const void *pixels, Texture *texture) {
}
void Vulkan_Backend::DestroyTexture(Texture *texture) {
}
void Vulkan_Backend::BeginFrame(f32 dt) {
}
void Vulkan_Backend::DrawGeometry() {
}
void Vulkan_Backend::EndFrame(f32 dt) {
}
}
