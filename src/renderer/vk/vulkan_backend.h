#include "renderer/renderer_backend.h"
#include "vulkan_render_device.h"
#include "vulkan_pipeline.h"
#include "vulkan_swapchain.h"
#include "vulkan_ds.h"

#include <memory>
#include <vector>

namespace xjar {

class Vulkan_Backend final : public RendererBackend {
public:
    Vulkan_Backend() = default;

    void        OnInit() override;
    void        OnDestroy() override;
    void        OnResized(u32 width, u32 height) override;
    void        UpdateGlobalState(const GPU_SceneData &sceneData) override;
    void        CreateTexture(const void *pixels, Texture *texture) override;
    void        DestroyTexture(Texture *texture) override;
    FrameStatus BeginFrame() override;
    void        EndFrame() override;
    void        CreateMesh(Model &model) override;
    void        BeginDefaultPass() override;
    void        EndDefaultPass() override;
    void        ClearColor(f32 r, f32 g, f32 b, f32 a) override;
    void       *GetDefaultRenderPass() override;
    void       *GetRenderDevice() override;

private:
    void CreateBuffers();
    void CreateDescriptors();
    void RecreateSwapchain();
    VkCommandBuffer *GetCurrentCommandBuffer();
#if 0
    DescriptorAllocator *GetCurrentFrameDescriptor();
#endif
    VkResult AcquireNextImage(u32 *imageIndex);

    Vulkan_RenderDevice                 m_renderDevice;
    std::unique_ptr<Vulkan_Swapchain>   m_swapchain;
    std::vector<VkCommandBuffer>        m_commandBuffers;
    //std::vector<DescriptorAllocator>    m_frameDescriptors;    
    u32                                 m_currentImageIndex;
    i32                                 m_currentFrameIndex = 0;
};
}
