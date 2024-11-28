#include "renderer/renderer_backend.h"

namespace xjar {

class OpenGL_Backend final : public RendererBackend {
public:
    OpenGL_Backend() = default;

    void OnInit() override;
    void OnDestroy() override;
    void OnResized(u32 width, u32 height) override;
    void UpdateGlobalState(const glm::mat4 &proj, const glm::mat4 &view) override;
    void CreateTexture(const void *pixels, Texture *texture) override;
    void DestroyTexture(Texture *texture) override;
    void BeginFrame(f32 dt);
    void DrawGeometry();
    void EndFrame(f32 dt);
};
}
