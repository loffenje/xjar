#pragma once

#include "vulkan_test_feature.h"
#include "vulkan_render_device.h"
#include "vulkan_pipeline.h"
#include "vulkan_model.h"
#include "io.h"
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "world.h"

namespace xjar {

struct PushConstantData {
    glm::mat4 transform;
};

void Vulkan_TestFeature::Init(void *device, void *renderPass) {
    m_renderDevice = (Vulkan_RenderDevice *)device;
    m_renderPass = *(VkRenderPass *)renderPass;

    CreatePipeline();
}

void Vulkan_TestFeature::CreatePipeline() {
    auto vertShaderCode = ReadFile("shaders/basic.vert.spv");
    auto fragShaderCode = ReadFile("shaders/basic.frag.spv");

    VkShaderModule vertShaderModule = CreateShaderModule(m_renderDevice, vertShaderCode);
    VkShaderModule fragShaderModule = CreateShaderModule(m_renderDevice, fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageInfo,
        fragShaderStageInfo};

    VkPushConstantRange push;
    push.offset = 0;
    push.size = sizeof(PushConstantData) * 2;
    push.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(Vertex3D);
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex3D, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex3D, color);

    m_pipeline.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    m_pipeline.SetPolygonMode(VK_POLYGON_MODE_FILL);
    m_pipeline.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    m_pipeline.SetMultisamplingNone();
    m_pipeline.DisableBlending();
    m_pipeline.EnableDepthtest(VK_TRUE, VK_COMPARE_OP_LESS);
    m_pipeline.SetPushConstants(push, 1);
    m_pipeline.SetVertexInput(bindingDescriptions, attributeDescriptions);
    m_pipeline.SetShaders(shaderStages);
    m_pipeline.Create(m_renderDevice, m_renderPass);

    vkDestroyShaderModule(m_renderDevice->device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_renderDevice->device, vertShaderModule, nullptr);
}

void Vulkan_TestFeature::DrawEntities(void *cmdbuf, const Camera &camera, std::initializer_list<Entity *> entities) {
    VkCommandBuffer *vkcmdbuf = (VkCommandBuffer *)cmdbuf;
    m_pipeline.Bind(*vkcmdbuf);

    for (Entity *ent : entities) {
        Vulkan_Model *vkmodel = (Vulkan_Model *)ent->model.handle;

        PushConstantData constants;
        constants.transform = camera.projection * camera.view * ent->model.localTransform;

        //upload the matrix to the GPU via push constants
        vkCmdPushConstants(*vkcmdbuf, m_pipeline.pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(PushConstantData), &constants);

        vkmodel->Bind(*vkcmdbuf);
        vkmodel->Draw(*vkcmdbuf);
    }
}

}
