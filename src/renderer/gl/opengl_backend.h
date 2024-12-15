#include <glad/glad.h>
#include "renderer/renderer_backend.h"

namespace xjar {

class OpenGL_Backend final : public RendererBackend {
public:
    OpenGL_Backend() = default;

    void        OnInit() override;
    void        OnDestroy() override;
    void        CreateMesh(Model &model) override;
    void        OnResized(u32 width, u32 height) override;
    void        UpdateGlobalState(const GPU_SceneData &sceneData) override;
    void        CreateTexture(const void *pixels, Texture *texture) override;
    void        DestroyTexture(Texture *texture) override;
    FrameStatus BeginFrame() override;
    void        BeginDefaultPass() override;
    void        EndDefaultPass() override;
    void        EndFrame() override;
};

}
