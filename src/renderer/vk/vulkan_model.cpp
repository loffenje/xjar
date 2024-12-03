#include "vulkan_model.h"
#include "vulkan_render_device.h"

namespace xjar {

void Vulkan_Model::Init(Vulkan_RenderDevice *_rd, const std::vector<Vertex2D> &vertices) {
    rd = _rd;

    vertexCount = static_cast<u32>(vertices.size());
    VkDeviceSize bufsize = sizeof(vertices[0]) * vertexCount;

    CreateBuffer(rd, bufsize,
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 vertexBuffer, vertexBufferMemory);

    void *data;
    vkMapMemory(rd->device, vertexBufferMemory, 0, bufsize, 0, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(bufsize));
    vkUnmapMemory(rd->device, vertexBufferMemory);
}

void Vulkan_Model::Bind(VkCommandBuffer cmdbuf) {
    VkBuffer     buffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmdbuf, 0, 1, buffers, offsets);
}

void Vulkan_Model::Draw(VkCommandBuffer cmdbuf) {
    vkCmdDraw(cmdbuf, vertexCount, 1, 0, 0);
}

}

