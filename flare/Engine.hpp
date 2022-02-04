#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <type_traits>
#include <fstream>
#include <memory>

#include <nlohmann/json.hpp>

#include <vulkan/vulkan.hpp>

#include "Log.hpp"

struct GLFWwindow;

namespace fve {

	class Device;
	class Shader;
	class Swapchain;
	class Pipeline;
	class Mesh;
	
	class Engine final {
	public:
		struct Settings {
			uint32_t width = 600;
			uint32_t height = 600;
			std::string shader = "";

			inline static void read(std::istream& is, Settings& settings) {
				nlohmann::json json;
				is >> json;
				from_json(json, settings);
			}

			inline static void write(std::ostream& os, const Settings& settings) {
				nlohmann::json json;
				to_json(json, settings);
				os << std::setw(4) << json;
			}

			inline static bool load(const std::string& filepath, Settings& settings) noexcept {
				try {
					std::fstream file{ filepath, std::ios::in | std::ios::binary };
					if (!file.is_open()) {
						Log_error("failed to open file {} for reading", filepath);
						return false;
					}
					Settings::read(file, settings);
					return true;
				}
				catch (const std::exception& ex) {
					Log_error("failed to load settings from file {}. error {}", filepath, ex.what());
				}
				catch (...) {
					Log_error("failed to load settings from file {}. unknown error", filepath);
				}
				return false;
			}

			inline static bool save(const std::string& filepath, const Settings& settings) noexcept {
				try {
					std::fstream file{ filepath, std::ios::out | std::ios::binary };
					if (!file.is_open()) {
						Log_error("failed to open file {} for writing", filepath);
						return false;
					}
					Settings::write(file, settings);
					return true;
				}
				catch (const std::exception& ex) {
					Log_error("failed to save settings into file {}. error {}", filepath, ex.what());
				}
				catch (...) {
					Log_error("failed to save settings into file {}. unknown error", filepath);
				}
				return false;
			}

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(Settings, width, height, shader)
		};

		explicit Engine(int argc, char** argv);

		Engine(const Engine&) = delete;
		Engine& operator=(const Engine&) = delete;

		~Engine() noexcept;

		static Engine* get() noexcept;

		int32_t run() noexcept;

		template<typename T>
		auto getSystem() const noexcept {
			try {
				auto it = std::find_if(systems_.begin(), systems_.end(), [](const std::unique_ptr<System>& other) { 
					return typeid(T).hash_code() == typeid(*other.get()).hash_code(); 
				});
				if (it != systems_.end())
					return static_cast<T*>((*it).get());
				Log_error("failed to get system {}. system {} doesn't exist", typeid(T).name(), typeid(T).name());
				return (T*)nullptr;
			}
			catch (const std::exception& ex) {
				Log_error("failed to get system {}. error", typeid(T).name(), ex.what());
			}
			catch (...) {
				Log_error("failed to get system {}. unknown error", typeid(T).name());
			}
			return (T*)nullptr;
		}

		Settings settings;

		std::shared_ptr<Shader> getShader(const std::string& shaderName) noexcept;

		std::shared_ptr<Shader> createShaderFromBinary(const std::string& shaderName,
													   const std::vector<uint32_t>& shaderBinary,
													   vk::ShaderStageFlagBits shaderStage) noexcept;

		std::shared_ptr<Shader> createShaderFromSource(const std::string& shaderName,
													   const std::string& shaderSource,
													   vk::ShaderStageFlagBits shaderStage) noexcept;

		std::vector<uint32_t> compileShaderSource(const std::string& shaderSource,
												  const std::string& shaderName,
												  vk::ShaderStageFlagBits shaderStage,
												  bool optimize = true);
	
	private:
		bool load() noexcept;
		bool unload() noexcept;

		void mainLoop();

		vk::CommandBuffer beginFrame() noexcept;
		void beginRenderPass(vk::CommandBuffer commandBuffer) noexcept;
		void endRenderPass(vk::CommandBuffer commandBuffer) noexcept;
		void endFrame(vk::CommandBuffer commandBuffer) noexcept;
		void drawFrame(vk::CommandBuffer commandBuffer);
		void reset();

		GLFWwindow* window_ = nullptr;
		std::unique_ptr<Device> device_ = nullptr;
		std::unique_ptr<Mesh> canvas_ = nullptr;
		std::unordered_map<std::string, std::shared_ptr<Shader>> shaders_;
		// renderer
		vk::UniquePipelineLayout pipelineLayout_;
		std::unique_ptr<Swapchain> swapchain_ = nullptr;
		std::unique_ptr<Pipeline> pipeline_ = nullptr;
		std::vector<vk::CommandBuffer> commandBuffers_;
		uint32_t currentImageIndex_ = 0;

	};

}
