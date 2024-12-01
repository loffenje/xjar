#include "vulkan_backend.h"
#include "vulkan_swapchain.h"
#include <glm/mat4x4.hpp>
#include <array>
#include "window.h"

#include "types.h"
#if RENDERER_BACKEND == OpenGL
#include "glfw_gl.h"
#else
#include "glfw_vk.h"
#endif

namespace xjar {

void Vulkan_Backend::OnInit() {
    m_renderDevice = CreateRenderDevice("xjar", "xjarEngine");
    RecreateSwapchain();
    CreateBuffers();
}

void Vulkan_Backend::CreateBuffers() {
    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_renderDevice.commandPool;
    allocInfo.commandBufferCount = static_cast<u32>(m_commandBuffers.size());

    if (vkAllocateCommandBuffers(m_renderDevice.device, &allocInfo, m_commandBuffers.data()) !=
        VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate buffers\n");
        exit(1);
    }
}

void Vulkan_Backend::OnDestroy() {
    vkDeviceWaitIdle(m_renderDevice.device);
    vkFreeCommandBuffers(
        m_renderDevice.device,
        m_renderDevice.commandPool,
        static_cast<u32>(m_commandBuffers.size()),
        m_commandBuffers.data());
    m_commandBuffers.clear();

    DestroySwapchain(m_swapchain.get(), m_renderDevice);
    DestroyRenderDevice(m_renderDevice);
}
void Vulkan_Backend::OnResized(u32 width, u32 height) {
    auto &window = GetWindow();
    window.width = width;
    window.height = height;
    window.resized = true;
}
void Vulkan_Backend::UpdateGlobalState(const glm::mat4 &proj, const glm::mat4 &view) {
}
void Vulkan_Backend::CreateTexture(const void *pixels, Texture *texture) {
}
void Vulkan_Backend::DestroyTexture(Texture *texture) {
}

void Vulkan_Backend::RecreateSwapchain() {
    auto       window = GetWindow();
    VkExtent2D extent = {window.width, window.height};
    while (extent.width == 0 || extent.height == 0) {
        extent = {window.width, window.height};
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(m_renderDevice.device);

    if (m_swapchain == nullptr) {
        m_swapchain = CreateSwapchain(m_renderDevice, extent, nullptr);
    } else {
        std::shared_ptr<Vulkan_Swapchain> oldSwapchain = std::move(m_swapchain);
        m_swapchain = CreateSwapchain(m_renderDevice, extent, oldSwapchain);
        DestroySwapchain(oldSwapchain.get(), m_renderDevice);
    }
}

bool Vulkan_Backend::BeginFrame(f32 dt) {
    VkResult result = AcquireNextImage(m_swapchain.get(), m_renderDevice, &m_currentImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapchain();
        return false;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        fprintf(stderr, "Failed to acquire swapchain image\n");
        exit(1);
    }

    auto cmdbuf = GetCurrentCommandBuffer();

    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(cmdbuf, &beginInfo) != VK_SUCCESS) {
        fprintf(stderr, "Failed to begin recording\n");
        exit(1);
    }

    return true;
}

void Vulkan_Backend::DrawGeometry() {
}
void Vulkan_Backend::EndFrame(f32 dt) {
    auto cmdbuf = GetCurrentCommandBuffer();
    if (vkEndCommandBuffer(cmdbuf) != VK_SUCCESS) {
        fprintf(stderr, "Failed to end recording\n");
        exit(1);
    }

    VkResult result = SubmitCommandBuffers(m_swapchain.get(), m_renderDevice, &cmdbuf, &m_currentImageIndex);
    auto &window = GetWindow();
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.resized) {
        window.resized = false;
        RecreateSwapchain();
    } else if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to present swapchain image\n");
        exit(1);
    }

    m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Vulkan_Backend::BeginSwapchainPass() {
    auto cmdbuf = GetCurrentCommandBuffer();

    VkRenderPassBeginInfo passInfo {};
    passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    passInfo.renderPass = m_swapchain->renderPass;
    passInfo.framebuffer = m_swapchain->framebuffers[m_currentImageIndex];

    passInfo.renderArea.offset = {0, 0};
    passInfo.renderArea.extent = m_swapchain->swapchainExtent;

    std::array<VkClearValue, 2> clearValues {};
    clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    passInfo.clearValueCount = static_cast<u32>(clearValues.size());
    passInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmdbuf, &passInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<f32>(m_swapchain->swapchainExtent.width);
    viewport.height = static_cast<f32>(m_swapchain->swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor {{0, 0}, m_swapchain->swapchainExtent};

    vkCmdSetViewport(cmdbuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdbuf, 0, 1, &scissor);
}

void Vulkan_Backend::EndSwapchainPass() {
    auto cmdbuf = GetCurrentCommandBuffer();

    vkCmdEndRenderPass(cmdbuf);
}

VkCommandBuffer Vulkan_Backend::GetCurrentCommandBuffer() const {
    return m_commandBuffers[m_currentFrameIndex];
}

}
