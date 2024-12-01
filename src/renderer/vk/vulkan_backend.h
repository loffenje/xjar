#include "renderer/renderer_backend.h"
#include "vulkan_render_device.h"
#include "vulkan_pipeline.h"
#include "vulkan_swapchain.h"

#include <memory>
#include <vector>

namespace xjar {

class Vulkan_Backend final : public RendererBackend {
public:
    Vulkan_Backend() = default;

    void OnInit() override;
    void OnDestroy() override;
    void OnResized(u32 width, u32 height) override;
    void UpdateGlobalState(const glm::mat4 &proj, const glm::mat4 &view) override;
    void CreateTexture(const void *pixels, Texture *texture) override;
    void DestroyTexture(Texture *texture) override;
    bool BeginFrame(f32 dt) override;
    void DrawGeometry();
    void EndFrame(f32 dt) override;
    
    void BeginSwapchainPass() override;
    void EndSwapchainPass() override;
private:
    void CreateBuffers();
    void RecreateSwapchain();
    VkCommandBuffer GetCurrentCommandBuffer() const;

    Vulkan_RenderDevice                 m_renderDevice;
    std::unique_ptr<Vulkan_Swapchain>   m_swapchain;
    std::vector<VkCommandBuffer>        m_commandBuffers;

    u32                                 m_currentImageIndex;
    i32                                 m_currentFrameIndex = 0;
};
}
