#include "Engine.hpp"
#include "Device.hpp"
#include "Shader.hpp"
#include "Swapchain.hpp"
#include "Pipeline.hpp"
#include "Mesh.hpp"
#include "Log.hpp"

#include <chrono>
#include <fstream>
#include <filesystem>
#include <stdexcept>

#include <flare_config.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <shaderc/shaderc.hpp>

namespace fve {

	static Engine* engineInstance = nullptr;

	struct GlobalConstant {
		glm::vec2 resolution;
		float time;
	};

	Engine::Engine(int /*argc*/, char** /*argv*/) {
		if (engineInstance)
			throw std::runtime_error{ "failed to initialize engine instance. engine instance already exists" };
		engineInstance = this;
	}

	Engine::~Engine() noexcept {
		engineInstance = nullptr;
	}

	Engine* Engine::get() noexcept {
		return engineInstance;
	}

	int32_t Engine::run() noexcept {
		if (!load())
			return EXIT_FAILURE;

		try {
			mainLoop();
		}
		catch (const vk::SystemError& err) {
			Log_error("main loop system error{}", err.what());
			return EXIT_FAILURE;
		}
		catch (const std::exception& ex) {
			Log_error("main loop error {}", ex.what());
			return EXIT_FAILURE;
		}
		catch (...) {
			Log_error("main loop unknown error");
			return EXIT_FAILURE;
		}

		if (!unload())
			return EXIT_FAILURE;
		return EXIT_SUCCESS;
	}

	std::shared_ptr<Shader> Engine::getShader(const std::string& shaderName) noexcept {
		if (auto it = shaders_.find(shaderName); it != shaders_.end())
			return it->second;
		return nullptr;
	}

	std::shared_ptr<Shader> Engine::createShaderFromBinary(const std::string& shaderName, const std::vector<uint32_t>& shaderBinary, vk::ShaderStageFlagBits shaderStage) noexcept {
		if (auto shader = getShader(shaderName))
			return shader;

		vk::ShaderModuleCreateInfo shaderModuleCreateInfo{};
		shaderModuleCreateInfo.setCode(shaderBinary);

		try {
			vk::ShaderModule shaderModule = device_->logical().createShaderModule(shaderModuleCreateInfo);
			auto shader = std::shared_ptr<Shader>(new Shader{ *device_, shaderModule, shaderStage });
			shaders_.insert({ shaderName, shader });
			return shader;
		}
		catch (const vk::SystemError& err) {
			Log_error("failed to create shader. error {}", err.what());
		}
		catch (const std::exception& ex) {
			Log_error("failed to create shader. error {}", ex.what());
		}
		catch (...) {
			Log_error("failed to create shader. unknown error");
		}

		return nullptr;
	}

	std::shared_ptr<Shader> Engine::createShaderFromSource(const std::string& shaderName, const std::string& shaderSource, vk::ShaderStageFlagBits shaderStage) noexcept {
		if (auto shader = getShader(shaderName))
			return shader;

		try {
			auto shaderBinary = compileShaderSource(shaderSource, shaderName, shaderStage);
			if (shaderBinary.empty())
				return nullptr;

			return createShaderFromBinary(shaderName, shaderBinary, shaderStage);
		}
		catch (const std::exception& ex) {
			Log_error("failed to create shader. error {}", ex.what());
		}
		catch (...) {
			Log_error("failed to create shader. unknown error");
		}

		return nullptr;
	}

	std::vector<uint32_t> Engine::compileShaderSource(const std::string& shaderSource, const std::string& shaderName, vk::ShaderStageFlagBits shaderStage, bool optimize) {
		shaderc_shader_kind kind;
		switch (shaderStage) {
		case vk::ShaderStageFlagBits::eVertex:
			kind = shaderc_vertex_shader;
			break;
		case vk::ShaderStageFlagBits::eFragment:
			kind = shaderc_fragment_shader;
			break;
		default:
			Log_error("failed to compile shader source. unsupported shader stage {}", vk::to_string(shaderStage));
			return {};
		}

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		if (optimize)
			options.SetOptimizationLevel(shaderc_optimization_level_size);

		shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(shaderSource, kind, shaderName.c_str(), options);

		if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
			Log_error("failed to compile shader source. error {}", module.GetErrorMessage());
			return {};
		}

		return { module.cbegin(), module.cend() };
	}

	bool Engine::load() noexcept {
		try {
			Log_info("{} {} {}.{}.{}", flare_PROJECT, flare_REVISION, flare_VERSION_MAJOR, flare_VERSION_MINOR, flare_VERSION_PATCH);

			const auto filepath = "flare.json";
			if (!Settings::load(filepath, settings)) {
				Log_warn("failed to load settings from file {}. skip to default", filepath);
				Settings::save(filepath, settings);
			}

			if (!glfwInit())
				throw std::runtime_error{ "failed to initialize GLFW" };

			glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
			glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

			std::stringstream ss;
			ss << flare_PROJECT << " "
				<< flare_VERSION_MAJOR << "."
				<< flare_VERSION_MINOR << "."
				<< flare_VERSION_PATCH << " "
				<< flare_REVISION;

			window_ = glfwCreateWindow(settings.width, settings.height, ss.str().c_str(), nullptr, nullptr);

			if (!window_)
				throw std::runtime_error{ "failed to create GLFW window" };

			GLFWimage icons[1];
			icons[0].pixels = stbi_load("icons/flare.png", &icons[0].width, &icons[0].height, 0, STBI_default);
			glfwSetWindowIcon(window_, 1, icons);
			stbi_image_free(icons[0].pixels);

			device_ = std::make_unique<Device>(window_);

			auto readFile = [](const std::filesystem::path& filepath, std::vector<uint32_t>& buffer) {
				std::ifstream file{ filepath, std::ios::in | std::ios::binary };
				if (!file.is_open())
					return false;
				try {
					file.seekg(0, std::ios::end);
					const size_t size = static_cast<size_t>(file.tellg());
					file.seekg(0, std::ios::beg);
					buffer.resize(size / 4, 0);
					file.read(reinterpret_cast<char*>(buffer.data()), size);
					return true;
				}
				catch (const std::exception& ex) {
					Log_error("failed to read from the file {}. error {}", filepath.string(), ex.what());
				}
				catch (...) {
					Log_error("failed to read from the file {}. unknown error.", filepath.string());
				}
				return false;
			};

			for (const auto& entry : std::filesystem::directory_iterator("shaders")) {
				// trying to find previous file extension to determine shader stage
				bool supported = false;

				auto origin = entry.path().filename().stem();
				vk::ShaderStageFlagBits shaderStage{};
				if (origin.extension() == ".vert")
					shaderStage = vk::ShaderStageFlagBits::eVertex; supported = true;
				if (origin.extension() == ".frag")
					shaderStage = vk::ShaderStageFlagBits::eFragment; supported = true;
				if (!supported)
					continue;
				std::vector<uint32_t> shaderBinary{};
				if (readFile(entry.path(), shaderBinary) && !shaderBinary.empty()) {
					if (!createShaderFromBinary(origin.string(), shaderBinary, shaderStage))
						Log_error("failed to load shader {} from binary file", origin.string());
				}
			}

			const float side = 1.0f;

			std::vector<Mesh::Vertex> vertices = {
				{{-side, side, 0.0f}},
				{{side, side, 0.0f}},
				{{side, -side, 0.0f}},
				{{-side, -side, 0.0f}},
			};

			std::vector<Mesh::Index> indices = {
				0, 1, 2, 2, 3, 0
			};

			canvas_ = std::make_unique<Mesh>(*device_, vertices, indices);

			vk::PushConstantRange pushConstantRange{};
			pushConstantRange.setOffset(0);
			pushConstantRange.setStageFlags(vk::ShaderStageFlagBits::eFragment);
			pushConstantRange.setSize(sizeof(GlobalConstant));

			vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
			pipelineLayoutCreateInfo.setPushConstantRanges(pushConstantRange);

			try {
				pipelineLayout_ = device_->logical().createPipelineLayoutUnique(pipelineLayoutCreateInfo);
			}
			catch (const vk::SystemError& err) {
				Log_error("failed to create vulkan pipeline layout. error {}", err.what());
				return false;
			}

			int w, h;
			glfwGetFramebufferSize(window_, &w, &h);
			swapchain_ = std::make_unique<Swapchain>(*device_, vk::Extent2D{ static_cast<uint32_t>(w), static_cast<uint32_t>(h) });

			const auto& canvasSource = R"glsl(
				#version 450
				#extension GL_ARB_separate_shader_objects : enable
				
				layout(location = 0) in vec3 inPos;
				
				void main() {
				    gl_Position = vec4(inPos, 1.0);
				}
			)glsl";

			auto vert = createShaderFromSource("canvas.vert", canvasSource, vk::ShaderStageFlagBits::eVertex);

			auto frag = getShader(settings.shader);
			if (!frag) {
				const auto& defaultSource = R"glsl(
					#version 450
					#extension GL_ARB_separate_shader_objects : enable
					
					layout(location = 0) out vec4 fragColor;

					layout(push_constant) uniform globalConstant {
					    vec2 resolution;
						float time;
					} global;
					
					void main() {
						vec3 col = vec3((0.5*sin(global.time) + 0.5), (0.5*cos(global.time) + 0.5), 0.8);
						
					    fragColor = vec4(col, 1.0);
					}
				)glsl";

				frag = createShaderFromSource("default.frag", defaultSource, vk::ShaderStageFlagBits::eFragment);
			}

			Pipeline::Settings pipelineSettings{};
			Pipeline::defaultPipelineSettings(pipelineSettings);
			pipelineSettings.pipelineLayout = *pipelineLayout_;
			pipelineSettings.renderPass = swapchain_->renderPass();
			pipelineSettings.bindingDescriptions = Mesh::Vertex::bindingDescriptions();
			pipelineSettings.attributeDescriptions = Mesh::Vertex::attributeDescriptions();

			pipeline_ = std::make_unique<Pipeline>(*device_, std::vector<std::shared_ptr<Shader>>{vert, frag}, pipelineSettings);

			commandBuffers_.resize(swapchain_->size());

			vk::CommandBufferAllocateInfo commandBufferAllocateInfo{};
			commandBufferAllocateInfo.setCommandPool(device_->commandPool());
			commandBufferAllocateInfo.setLevel(vk::CommandBufferLevel::ePrimary);
			commandBufferAllocateInfo.setCommandBufferCount(static_cast<uint32_t>(commandBuffers_.size()));

			try {
				commandBuffers_ = device_->logical().allocateCommandBuffers(commandBufferAllocateInfo);
			}
			catch (const vk::SystemError& err) {
				Log_error("failed to allocate vulkan command buffers. error {}", err.what());
				return false;
			}

			return true;
		}
		catch (const std::exception& ex) {
			Log_error("failed to load engine. error {}", ex.what());
		}
		catch (...) {
			Log_error("failed to load engine. unknown error");
		}
		return false;
	}

	bool Engine::unload() noexcept {
		try {
			glfwDestroyWindow(window_);
			glfwTerminate();

			return true;
		}
		catch (const std::exception& ex) {
			Log_error("failed to unload engine. error {}", ex.what());
		}
		catch (...) {
			Log_error("failed to unload engine. unknown error");
		}
		return false;
	}

	void Engine::mainLoop() {
		auto currentTime = std::chrono::high_resolution_clock::now();

		glfwShowWindow(window_);
		while (!glfwWindowShouldClose(window_)) {
			auto newTime = std::chrono::high_resolution_clock::now();
			float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
			currentTime = newTime;

			glfwPollEvents();
			if (glfwGetKey(window_, GLFW_KEY_ESCAPE))
				glfwSetWindowShouldClose(window_, true);

			auto cb = beginFrame();
			beginRenderPass(cb);
			drawFrame(cb);
			endRenderPass(cb);
			endFrame(cb);

			glfwSwapBuffers(window_);
		}

		device_->logical().waitIdle();
	}

	vk::CommandBuffer Engine::beginFrame() noexcept {
		if (swapchain_->acquireNextImage(currentImageIndex_) != vk::Result::eSuccess) {
			Log_error("failed to acquire next image from the swapchain");
			return {};
		}
		auto cb = commandBuffers_[currentImageIndex_];
		try {
			cb.begin(vk::CommandBufferBeginInfo{});
			return cb;
		}
		catch (const vk::SystemError& err) {
			Log_error("failed to begin command buffer {}. error {}", currentImageIndex_, err.what());
		}
		catch (const std::exception& ex) {
			Log_error("failed to begin command buffer {}. error {}", currentImageIndex_, ex.what());
		}
		catch (...) {
			Log_error("failed to begin command buffer {}. unknown error", currentImageIndex_);
		}
		return {};
	}

	void Engine::beginRenderPass(vk::CommandBuffer commandBuffer) noexcept {
		vk::ClearValue clearColor = { std::array<float, 4>{ 0.1f, 0.1f, 0.1f, 1.0f } };

		vk::RenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.setRenderPass(swapchain_->renderPass());
		renderPassBeginInfo.setFramebuffer(swapchain_->framebuffer(currentImageIndex_));
		renderPassBeginInfo.setClearValues(clearColor);
		renderPassBeginInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
		renderPassBeginInfo.renderArea.extent = swapchain_->extent();

		commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
	}

	void Engine::endRenderPass(vk::CommandBuffer commandBuffer) noexcept {
		commandBuffer.endRenderPass();
	}

	void Engine::endFrame(vk::CommandBuffer commandBuffer) noexcept {
		try {
			commandBuffer.end();
		}
		catch (const vk::SystemError& err) {
			Log_error("failed to end command buffer. error {}", err.what());
			return;
		}

		if (swapchain_->submit(commandBuffer, currentImageIndex_) != vk::Result::eSuccess) 
			Log_error("failed to submit command buffer");
	}

	void Engine::drawFrame(vk::CommandBuffer commandBuffer) {
		pipeline_->bind(commandBuffer);

		vk::Viewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapchain_->extent().width);
		viewport.height = static_cast<float>(swapchain_->extent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vk::Rect2D scissor{ {0, 0}, swapchain_->extent() };
		commandBuffer.setViewport(0, viewport);
		commandBuffer.setScissor(0, scissor);

		GlobalConstant global{};
		global.resolution = { viewport.width, viewport.height };
		global.time = static_cast<float>(glfwGetTime());

		commandBuffer.pushConstants(*pipelineLayout_, vk::ShaderStageFlagBits::eFragment, 0, sizeof(GlobalConstant), &global);

		canvas_->bind(commandBuffer);
		canvas_->draw(commandBuffer);
	}

}

int main(int argc, char** argv) {
	return fve::Engine{ argc, argv }.run();
}