#pragma once

#include <vector>
#include <memory>

#include <vulkan/vulkan.hpp>

namespace fve {

	class Device;
	class Shader;

	class Pipeline final {
	public:
		struct Settings {
			Settings() = default;
			~Settings() = default;

			Settings(const Settings&) = delete;
			Settings& operator=(const Settings&) = delete;

			vk::PipelineViewportStateCreateInfo viewportStateCreateInfo{};
			vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
			vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
			vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
			vk::PipelineColorBlendAttachmentState colorBlendAttachmentState{};
			vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
			std::vector<vk::DynamicState> dynamicStates{};
			std::vector<vk::VertexInputBindingDescription> bindingDescriptions{};
			std::vector<vk::VertexInputAttributeDescription> attributeDescriptions{};
			vk::PipelineLayout pipelineLayout = nullptr;
			vk::RenderPass renderPass = nullptr;
			uint32_t subpass = 0;
		};

		explicit Pipeline(Device& device, const std::vector<std::shared_ptr<Shader>>& shaders, const Settings& settings);

		~Pipeline() noexcept;

		Pipeline(const Pipeline&) = delete;
		Pipeline& operator=(const Pipeline&) = delete;

		void bind(vk::CommandBuffer commandBuffer);

		static void defaultPipelineSettings(Settings& settings) noexcept;

	private:
		vk::UniquePipeline pipeline_;
	};

}