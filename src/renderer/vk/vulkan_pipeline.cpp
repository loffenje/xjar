#include "vulkan_pipeline.h"
#include "vulkan_render_device.h"

namespace xjar {

void Vulkan_Pipeline::Reset() {
	inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

	rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

	pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	colorBlendAttachment = {};

	multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipelineLayout = {};
	depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

	shaderStages.clear();
}

void Vulkan_Pipeline::Create(Vulkan_RenderDevice &rd, VkViewport viewport, VkRenderPass renderPass) {
	// filter, which pixels will be stored within this region, others will be discarded
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
    // TODO(vadym): fix it
	scissor.extent = VkExtent2D{ .width = 1920, .height = 1080 };


	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.pScissors = &scissor;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.pNext = nullptr;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	if (vkCreatePipelineLayout(rd.device,
		&pipelineLayoutInfo,
		nullptr,
		&pipelineLayout
	) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create pipeline\n");
		exit(EXIT_FAILURE);
	}

	VkGraphicsPipelineCreateInfo graphicsPipelineInfo{};
	graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineInfo.stageCount = (uint32_t)shaderStages.size();
	graphicsPipelineInfo.pStages = shaderStages.data();
	graphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
	graphicsPipelineInfo.pInputAssemblyState = &inputAssembly;
	graphicsPipelineInfo.pViewportState = &viewportState;
	graphicsPipelineInfo.pRasterizationState = &rasterizer;
	graphicsPipelineInfo.pMultisampleState = &multisampling;
	graphicsPipelineInfo.pColorBlendState = &colorBlending;
	graphicsPipelineInfo.pDepthStencilState = &depthStencil;
	graphicsPipelineInfo.layout = pipelineLayout;
	graphicsPipelineInfo.renderPass = renderPass;
	graphicsPipelineInfo.subpass = 0;

	VkDynamicState state[] = { VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicInfo{};
	dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicInfo.pDynamicStates = &state[0];
	dynamicInfo.dynamicStateCount = 1;

	graphicsPipelineInfo.pDynamicState = &dynamicInfo;

	if (vkCreateGraphicsPipelines(rd.device, VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		fprintf(stderr, "failed to create pipeline\n");
		pipeline = VK_NULL_HANDLE;
	}
}

void Vulkan_Pipeline::Destroy(VkDevice device) {
	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

	pipeline = {};
	pipelineLayout = {};
}

void Vulkan_Pipeline::SetShaders(std::span<VkPipelineShaderStageCreateInfo> _shaderStages) {
	shaderStages.clear();

	shaderStages.resize(_shaderStages.size());
	std::copy(_shaderStages.begin(), _shaderStages.end(), shaderStages.begin());
}

void Vulkan_Pipeline::SetDescriptorSets(VkDescriptorSetLayout *layouts, u32 layoutsNum) {
	pipelineLayoutInfo.setLayoutCount = layoutsNum;
	pipelineLayoutInfo.pSetLayouts = layouts;
}

void Vulkan_Pipeline::SetInputTopology(VkPrimitiveTopology topology) {
	inputAssembly.topology = topology;
	inputAssembly.primitiveRestartEnable = VK_FALSE;
}

void Vulkan_Pipeline::SetVertexInput(VkVertexInputBindingDescription bindingDescription,
	std::span<VkVertexInputAttributeDescription> attributeDescriptions) {
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<u32>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
}

void Vulkan_Pipeline::SetPolygonMode(VkPolygonMode mode) {
	rasterizer.polygonMode = mode;
	rasterizer.lineWidth = 1.0f;
}

void Vulkan_Pipeline::SetCullMode(VkCullModeFlags mode, VkFrontFace front_face) {
	rasterizer.cullMode = mode;
	rasterizer.frontFace = front_face;
}

// anti-aliasing, combination of fragment shaders of multiple polygons that rasterize to the same pixel
void Vulkan_Pipeline::SetMultisamplingNone() {
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;
}

void Vulkan_Pipeline::DisableBlending() {
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
}

void Vulkan_Pipeline::DisableDepthtest() {
	depthStencil.depthTestEnable = VK_FALSE;
	depthStencil.depthWriteEnable = VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {};
	depthStencil.back = {};
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
}

void Vulkan_Pipeline::EnableDepthtest(bool writeEnable, VkCompareOp op) {
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = writeEnable;
	depthStencil.depthCompareOp = op;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {};
	depthStencil.back = {};
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
}

// out color = src color.rgb * src color.a + dst color.rgb * 1.0
void Vulkan_Pipeline::EnableBlendingAdditive() {
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

// out color = src color.rgb * src color.a + dst color.rgb * (1.0 - src color.a)
void Vulkan_Pipeline::EnableBlendingAlphablend() {
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

void Vulkan_Pipeline::Bind(VkCommandBuffer cmd, VkPipelineBindPoint point) {
	vkCmdBindPipeline(cmd, point, pipeline);
}

}

