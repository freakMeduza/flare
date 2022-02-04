#pragma once

#include <vulkan/vulkan.hpp>

#include "Device.hpp"

namespace fve {

	class Shader final {
		friend class Engine;

	public:
		~Shader() noexcept {
			device_.logical().destroyShaderModule(shaderModule_);
		}

		Shader(const Shader&) = delete;
		Shader& operator=(const Shader&) = delete;

		inline vk::ShaderModule shaderModule() const noexcept { return shaderModule_; }
		inline vk::ShaderStageFlagBits shaderStage() const noexcept { return shaderStage_; }

	private:
		explicit Shader(Device& device, vk::ShaderModule shaderModule, vk::ShaderStageFlagBits shaderStage) :
			device_{ device }, shaderModule_{ shaderModule }, shaderStage_{ shaderStage }
		{}

		Device& device_;
		vk::ShaderModule shaderModule_;
		vk::ShaderStageFlagBits shaderStage_;
	};

}