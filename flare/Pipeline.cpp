#include "Pipeline.hpp"
#include "Device.hpp"
#include "Shader.hpp"
#include "Log.hpp"

namespace {
	static constexpr const char* SHADER_ENTRY_POINT = "main";
}

namespace fve {

	Pipeline::Pipeline(Device& device, const std::vector<std::shared_ptr<Shader>>& shaders, const Settings& settings) {
		std::vector<vk::PipelineShaderStageCreateInfo> shaderStageCreateInfos{};
		for (auto shader : shaders) {
			vk::PipelineShaderStageCreateInfo shaderStageCreateInfo{};
			shaderStageCreateInfo.setModule(shader->shaderModule());
			shaderStageCreateInfo.setStage(shader->shaderStage());
			shaderStageCreateInfo.setPName(SHADER_ENTRY_POINT);
			shaderStageCreateInfos.emplace_back(shaderStageCreateInfo);
		}

		vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
		vertexInputStateCreateInfo.setVertexBindingDescriptions(settings.bindingDescriptions);
		vertexInputStateCreateInfo.setVertexAttributeDescriptions(settings.attributeDescriptions);

		vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
		colorBlendStateCreateInfo.setLogicOpEnable(false);
		colorBlendStateCreateInfo.setLogicOp(vk::LogicOp::eCopy);
		colorBlendStateCreateInfo.setAttachmentCount(1);
		colorBlendStateCreateInfo.setPAttachments(&settings.colorBlendAttachmentState);
		colorBlendStateCreateInfo.setBlendConstants({ 0.f, 0.f, 0.f, 0.f });

		vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
		dynamicStateCreateInfo.setDynamicStates(settings.dynamicStates);
		dynamicStateCreateInfo.setFlags({});

		vk::GraphicsPipelineCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.setStages(shaderStageCreateInfos);
		pipelineCreateInfo.setPVertexInputState(&vertexInputStateCreateInfo);
		pipelineCreateInfo.setPInputAssemblyState(&settings.inputAssemblyStateCreateInfo);
		pipelineCreateInfo.setPViewportState(&settings.viewportStateCreateInfo);
		pipelineCreateInfo.setPRasterizationState(&settings.rasterizationStateCreateInfo);
		pipelineCreateInfo.setPMultisampleState(&settings.multisampleStateCreateInfo);
		pipelineCreateInfo.setPDepthStencilState(&settings.depthStencilStateCreateInfo);
		pipelineCreateInfo.setPColorBlendState(&colorBlendStateCreateInfo);
		pipelineCreateInfo.setPDynamicState(&dynamicStateCreateInfo);
		pipelineCreateInfo.setLayout(settings.pipelineLayout);
		pipelineCreateInfo.setRenderPass(settings.renderPass);
		pipelineCreateInfo.setSubpass(settings.subpass);

		pipeline_ = device.logical().createGraphicsPipelineUnique(nullptr, pipelineCreateInfo);
	}

	Pipeline::~Pipeline() noexcept {
	}

	void Pipeline::bind(vk::CommandBuffer commandBuffer) {
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline_);
	}

	void Pipeline::defaultPipelineSettings(Settings& settings) noexcept {
		settings.inputAssemblyStateCreateInfo.setTopology(vk::PrimitiveTopology::eTriangleList);
		settings.inputAssemblyStateCreateInfo.setPrimitiveRestartEnable(false);

		settings.viewportStateCreateInfo.setViewportCount(1);
		settings.viewportStateCreateInfo.setPViewports(nullptr);
		settings.viewportStateCreateInfo.setScissorCount(1);
		settings.viewportStateCreateInfo.setPScissors(nullptr);

		settings.rasterizationStateCreateInfo.setDepthClampEnable(false);
		settings.rasterizationStateCreateInfo.setRasterizerDiscardEnable(false);
		settings.rasterizationStateCreateInfo.setPolygonMode(vk::PolygonMode::eFill);
		settings.rasterizationStateCreateInfo.setLineWidth(1.f);
		settings.rasterizationStateCreateInfo.setCullMode(vk::CullModeFlagBits::eNone);
		settings.rasterizationStateCreateInfo.setFrontFace(vk::FrontFace::eClockwise);
		settings.rasterizationStateCreateInfo.setDepthBiasEnable(false);
		settings.rasterizationStateCreateInfo.setDepthBiasConstantFactor(0.f);
		settings.rasterizationStateCreateInfo.setDepthBiasClamp(0.f);
		settings.rasterizationStateCreateInfo.setDepthBiasSlopeFactor(0.f);

		settings.multisampleStateCreateInfo.setSampleShadingEnable(false);
		settings.multisampleStateCreateInfo.setRasterizationSamples(vk::SampleCountFlagBits::e1);
		settings.multisampleStateCreateInfo.setMinSampleShading(1.f);
		settings.multisampleStateCreateInfo.setPSampleMask(nullptr);
		settings.multisampleStateCreateInfo.setAlphaToCoverageEnable(false);
		settings.multisampleStateCreateInfo.setAlphaToOneEnable(false);

		settings.colorBlendAttachmentState.setColorWriteMask(vk::ColorComponentFlagBits::eR |
															 vk::ColorComponentFlagBits::eG |
															 vk::ColorComponentFlagBits::eB |
															 vk::ColorComponentFlagBits::eA);
		settings.colorBlendAttachmentState.setBlendEnable(false);
		settings.colorBlendAttachmentState.setSrcColorBlendFactor(vk::BlendFactor::eOne);
		settings.colorBlendAttachmentState.setDstColorBlendFactor(vk::BlendFactor::eZero);
		settings.colorBlendAttachmentState.setColorBlendOp(vk::BlendOp::eAdd);
		settings.colorBlendAttachmentState.setSrcAlphaBlendFactor(vk::BlendFactor::eOne);
		settings.colorBlendAttachmentState.setDstAlphaBlendFactor(vk::BlendFactor::eZero);
		settings.colorBlendAttachmentState.setAlphaBlendOp(vk::BlendOp::eAdd);

		settings.depthStencilStateCreateInfo.setDepthTestEnable(true);
		settings.depthStencilStateCreateInfo.setDepthWriteEnable(true);
		settings.depthStencilStateCreateInfo.setDepthCompareOp(vk::CompareOp::eLess);
		settings.depthStencilStateCreateInfo.setDepthBoundsTestEnable(false);
		settings.depthStencilStateCreateInfo.setMinDepthBounds(0.f);
		settings.depthStencilStateCreateInfo.setMaxDepthBounds(1.f);
		settings.depthStencilStateCreateInfo.setStencilTestEnable(false);
		settings.depthStencilStateCreateInfo.setFront({});
		settings.depthStencilStateCreateInfo.setBack({});

		settings.dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
	}

}