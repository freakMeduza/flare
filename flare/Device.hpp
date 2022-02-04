#pragma once

#include <vulkan/vulkan.hpp>

#include <optional>

#include "Log.hpp"

struct GLFWwindow;

namespace fve {

	class Device final {
	public:
		struct QueueFamilyIndices {
			inline static QueueFamilyIndices findQueueFamilyIndices(vk::PhysicalDevice device, vk::SurfaceKHR surface) noexcept {
				QueueFamilyIndices indices{};
				const auto& queueFamilyProperties = device.getQueueFamilyProperties();
				for (auto i = 0u; i < queueFamilyProperties.size(); ++i) {
					if (queueFamilyProperties[i].queueCount > 0 && queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics)
						indices.graphicsFamily = i;
					if (queueFamilyProperties[i].queueCount > 0 && device.getSurfaceSupportKHR(i, surface))
						indices.presentFamily = i;
					if (indices.isCompleted())
						break;
				}
				return indices;
			}

			inline bool isCompleted() const noexcept { return graphicsFamily.has_value() && presentFamily.has_value(); }

			std::optional<uint32_t> graphicsFamily;
			std::optional<uint32_t> presentFamily;
		};

		explicit Device(GLFWwindow* window);

		~Device() noexcept;

		Device(const Device&) = delete;
		Device& operator=(const Device&) = delete;

		inline vk::Instance instance() const noexcept { return *instance_; }
		inline vk::Device logical() const noexcept { return *logical_; };
		inline vk::PhysicalDevice physical() const noexcept { return physical_; }
		inline vk::SurfaceKHR surface() const noexcept { return *surface_; }
		inline vk::Queue graphicsQueue() const noexcept { return graphicsQueue_; }
		inline vk::Queue presentQueue() const noexcept { return presentQueue_; }
		inline vk::CommandPool commandPool() const noexcept { return *commandPool_; }

		vk::CommandBuffer Device::beginSingleTimeCommandBuffer();
		void Device::endSingleTimeCommandBuffer(vk::CommandBuffer commandBuffer);

		std::pair<vk::Buffer, vk::DeviceMemory> createBuffer(vk::DeviceSize instanceSize,
															 vk::DeviceSize instanceCount,
															 vk::BufferUsageFlags usageFlags,
															 vk::MemoryPropertyFlags memoryPropertyFlags) const noexcept;

		bool copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);

	private:
#ifdef _DEBUG
		bool VALIDATION_LAYERS_ENABLED = true;
#else
		bool VALIDATION_LAYERS_ENABLED = false;
#endif

		const std::vector<const char*> VALIDATION_LAYERS = {
			"VK_LAYER_KHRONOS_validation"
		};

		const std::vector<const char*> DEVICE_EXTENSIONS = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		inline uint32_t Device::findMemoryTypeIndex(uint32_t typeFilter, vk::MemoryPropertyFlags memoryPropertyFlags) const {
			vk::PhysicalDeviceMemoryProperties properties;
			physical_.getMemoryProperties(&properties);
			for (uint32_t i = 0; i < properties.memoryTypeCount; i++) {
				if ((typeFilter & (1 << i)) && (properties.memoryTypes[i].propertyFlags & memoryPropertyFlags) == memoryPropertyFlags)
					return i;
			}
			throw std::runtime_error("failed to find suitable memory type");
		}

		bool checkValidationLayersSupport() const noexcept;
		bool checkDeviceExtensionSupport(vk::PhysicalDevice device) const noexcept;

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
															VkDebugUtilsMessageTypeFlagsEXT messageType,
															const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
															void* pUserData);

		void createInstance();
		void createDevice();
		void createCommandPool();

		GLFWwindow* window_ = nullptr;
		vk::UniqueInstance instance_;
		vk::UniqueDebugUtilsMessengerEXT debugMessenger_;
		vk::UniqueSurfaceKHR surface_;
		vk::PhysicalDevice physical_;
		vk::UniqueDevice logical_;
		vk::Queue graphicsQueue_;
		vk::Queue presentQueue_;
		vk::UniqueCommandPool commandPool_;
	};

}