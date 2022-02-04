#include "Device.hpp"

#include <set>
#include <sstream>
#include <stdexcept>

#include <flare_config.h>

#include <GLFW/glfw3.h>

PFN_vkCreateDebugUtilsMessengerEXT pfnVkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(VkInstance instance,
															  const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
															  const VkAllocationCallbacks* pAllocator,
															  VkDebugUtilsMessengerEXT* pMessenger) {
	return pfnVkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(VkInstance instance,
														   VkDebugUtilsMessengerEXT messenger,
														   VkAllocationCallbacks const* pAllocator) {
	return pfnVkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}

namespace fve {

	Device::Device(GLFWwindow* window) : window_{ window } {
		createInstance();

		if (VALIDATION_LAYERS_ENABLED) {
			pfnVkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(instance_->getProcAddr("vkCreateDebugUtilsMessengerEXT"));
			if (!pfnVkCreateDebugUtilsMessengerEXT)
				throw std::runtime_error{ "failed to get pfnVkCreateDebugUtilsMessengerEXT" };
			pfnVkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(instance_->getProcAddr("vkDestroyDebugUtilsMessengerEXT"));
			if (!pfnVkDestroyDebugUtilsMessengerEXT)
				throw std::runtime_error{ "failed to get pfnVkDestroyDebugUtilsMessengerEXT" };

			vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};
			debugMessengerCreateInfo.setMessageSeverity(/*vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |*/
														vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
														vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
			debugMessengerCreateInfo.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
													vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
													vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);
			debugMessengerCreateInfo.setPfnUserCallback(&Device::debugCallback);

			try {
				debugMessenger_ = instance_->createDebugUtilsMessengerEXTUnique(debugMessengerCreateInfo);
			}
			catch (const vk::SystemError& err) {
				Log_error("failed to create vulkan debug utils messenger. error {}", err.what());
				throw;
			}
		}

		createDevice();
		createCommandPool();
	}

	Device::~Device() noexcept {

	}

	vk::CommandBuffer Device::beginSingleTimeCommandBuffer() {
		vk::CommandBufferAllocateInfo commandBufferAllocateInfo{}; 
		commandBufferAllocateInfo.setLevel(vk::CommandBufferLevel::ePrimary);
		commandBufferAllocateInfo.setCommandPool(*commandPool_);
		commandBufferAllocateInfo.setCommandBufferCount(1);

		vk::CommandBuffer commandBuffer;

		if (logical_->allocateCommandBuffers(&commandBufferAllocateInfo, &commandBuffer) != vk::Result::eSuccess)
			throw std::runtime_error{ "failed to begin single time command buffer. failed to create command buffer" };

		vk::CommandBufferBeginInfo commandBufferBeginInfo{};
		commandBufferBeginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		try {
			commandBuffer.begin(commandBufferBeginInfo);
		}
		catch (const vk::SystemError& err) {
			Log_error("failed to begin command buffer. error {}", err.what());
			throw;
		}

		return commandBuffer;
	}

	void Device::endSingleTimeCommandBuffer(vk::CommandBuffer commandBuffer) {
		commandBuffer.end();

		std::array<vk::CommandBuffer, 1> commandBuffers{ commandBuffer };

		vk::SubmitInfo submitInfo{};
		submitInfo.setCommandBuffers(commandBuffers);

		try {
			graphicsQueue_.submit(submitInfo);
		}
		catch (const vk::SystemError& err) {
			Log_error("failed to submit command buffer. error {}", err.what());
			throw;
		}

		graphicsQueue_.waitIdle();

		logical_->freeCommandBuffers(*commandPool_, 1, &commandBuffer);
	}

	bool Device::checkValidationLayersSupport() const noexcept {
		const auto& instanceLayerProperties = vk::enumerateInstanceLayerProperties();
		for (auto layerName : VALIDATION_LAYERS) {
			auto it = std::find_if(instanceLayerProperties.begin(),
								   instanceLayerProperties.end(),
								   [=](const vk::LayerProperties& properties) {
									   return std::strcmp(layerName, properties.layerName) == 0;
								   });
			if (it == instanceLayerProperties.end())
				return false;
		}
		return true;
	}

	std::pair<vk::Buffer, vk::DeviceMemory> Device::createBuffer(vk::DeviceSize instanceSize,
																 vk::DeviceSize instanceCount,
																 vk::BufferUsageFlags usageFlags,
																 vk::MemoryPropertyFlags memoryPropertyFlags) const noexcept {
		vk::BufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.size = instanceSize * instanceCount;
		bufferCreateInfo.usage = usageFlags;
		bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

		std::pair<vk::Buffer, vk::DeviceMemory> buffer;

		try {
			buffer.first = logical_->createBuffer(bufferCreateInfo);
		}
		catch (const vk::SystemError& err) {
			Log_error("failed to create vulkan buffer. error {}", err.what());
			return {};
		}

		const auto& memoryRequirements = logical_->getBufferMemoryRequirements(buffer.first);
		const auto& memoryTypeIndex = findMemoryTypeIndex(memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		vk::MemoryAllocateInfo memoryAllocateInfo{};
		memoryAllocateInfo.setAllocationSize(memoryRequirements.size);
		memoryAllocateInfo.setMemoryTypeIndex(memoryTypeIndex);

		try {
			buffer.second = logical_->allocateMemory(memoryAllocateInfo);
		}
		catch (const vk::SystemError& err) {
			Log_error("failed to allocate vulkan device memory. error {}", err.what());
			return {};
		}

		try {
			logical_->bindBufferMemory(buffer.first, buffer.second, 0);
		}
		catch (const vk::SystemError& err) {
			Log_error("failed to bind vulkan buffer memory. error {}", err.what());
			return {};
		}

		return buffer;
	}

	bool Device::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) {
		if (auto cmb = beginSingleTimeCommandBuffer()) {
			std::array<vk::BufferCopy, 1> copyRegions{ vk::BufferCopy{0, 0, size} };
			cmb.copyBuffer(srcBuffer, dstBuffer, copyRegions);
			endSingleTimeCommandBuffer(cmb);
			return true;
		}
		return false;
	}

	bool Device::checkDeviceExtensionSupport(vk::PhysicalDevice device) const noexcept {
		auto deviceExtensionProperties = device.enumerateDeviceExtensionProperties();
		std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());
		for (const auto& extension : deviceExtensionProperties)
			requiredExtensions.erase(extension.extensionName);
		return requiredExtensions.empty();
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL Device::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
		std::stringstream message;

		message << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity)) << ": "
			<< vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageType)) << ":\n";
		message << "\t"
			<< "messageIDName   = <" << pCallbackData->pMessageIdName << ">\n";
		message << "\t"
			<< "messageIdNumber = " << pCallbackData->messageIdNumber << "\n";
		message << "\t"
			<< "message         = <" << pCallbackData->pMessage << ">\n";
		if (0 < pCallbackData->queueLabelCount) {
			message << "\t"
				<< "Queue Labels:\n";
			for (uint8_t i = 0; i < pCallbackData->queueLabelCount; i++) {
				message << "\t\t"
					<< "labelName = <" << pCallbackData->pQueueLabels[i].pLabelName << ">\n";
			}
		}
		if (0 < pCallbackData->cmdBufLabelCount) {
			message << "\t"
				<< "CommandBuffer Labels:\n";
			for (uint8_t i = 0; i < pCallbackData->cmdBufLabelCount; i++) {
				message << "\t\t"
					<< "labelName = <" << pCallbackData->pCmdBufLabels[i].pLabelName << ">\n";
			}
		}
		if (0 < pCallbackData->objectCount) {
			message << "\t"
				<< "Objects:\n";
			for (uint8_t i = 0; i < pCallbackData->objectCount; i++) {
				message << "\t\t"
					<< "Object " << i << "\n";
				message << "\t\t\t"
					<< "objectType   = "
					<< vk::to_string(static_cast<vk::ObjectType>(pCallbackData->pObjects[i].objectType)) << "\n";
				message << "\t\t\t"
					<< "objectHandle = " << pCallbackData->pObjects[i].objectHandle << "\n";
				if (pCallbackData->pObjects[i].pObjectName) {
					message << "\t\t\t"
						<< "objectName   = <" << pCallbackData->pObjects[i].pObjectName << ">\n";
				}
			}
		}
		switch (static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity)) {
		case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
			Log_debug(message.str());
			break;
		case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
			Log_warn(message.str());
			break;
		case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
			Log_warn(message.str());
			break;
		case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
			Log_error(message.str());
			break;
		}

		return VK_FALSE;
	}

	void Device::createInstance() {
		if (VALIDATION_LAYERS_ENABLED && !checkValidationLayersSupport()) {
			Log_warn("validation layuers requested but not supported, disable validation layers");
			VALIDATION_LAYERS_ENABLED = false;
		}

		vk::ApplicationInfo applicationInfo{};
		applicationInfo.setApiVersion(VK_API_VERSION_1_2);
		applicationInfo.setApplicationVersion(VK_MAKE_VERSION(0, 0, 1));
		applicationInfo.setPApplicationName("Default");
		applicationInfo.setEngineVersion(VK_MAKE_VERSION(flare_VERSION_MAJOR, flare_VERSION_MINOR, flare_VERSION_PATCH));
		applicationInfo.setPEngineName(flare_PROJECT);

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::vector<const char*> requiredInstanceExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		if (VALIDATION_LAYERS_ENABLED)
			requiredInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		vk::InstanceCreateInfo instanceCreateInfo{};
		instanceCreateInfo.setPApplicationInfo(&applicationInfo);
		instanceCreateInfo.setPEnabledExtensionNames(requiredInstanceExtensions);
		if (VALIDATION_LAYERS_ENABLED)
			instanceCreateInfo.setPEnabledLayerNames(VALIDATION_LAYERS);

		try {
			instance_ = vk::createInstanceUnique(instanceCreateInfo, nullptr);
		}
		catch (const vk::SystemError& err) {
			Log_error("failed to create vulkan instance. error {}", err.what());
			throw;
		}

		VkSurfaceKHR surface;
		if (glfwCreateWindowSurface(*instance_, window_, nullptr, &surface) != VK_SUCCESS)
			throw std::runtime_error{ "failed to create vulkan window surface" };
		surface_ = vk::UniqueSurfaceKHR{ surface, *instance_ };

		auto isSuitable = [this](vk::PhysicalDevice device, vk::SurfaceKHR surface) {
			auto indices = QueueFamilyIndices::findQueueFamilyIndices(device, surface);
			return indices.isCompleted() && checkDeviceExtensionSupport(device);
		};

		const auto devices = instance_->enumeratePhysicalDevices();
		if (devices.empty())
			throw std::runtime_error{ "failed to pick physical device. there are devices with vulkan support" };
		for (auto d : devices) {
			if (isSuitable(d, *surface_)) {
				physical_ = d;
				const auto id = physical_.getProperties().deviceID;
				const auto name = physical_.getProperties().deviceName;
				const auto apiVersion = physical_.getProperties().apiVersion;
				const auto driverVersion = physical_.getProperties().driverVersion;
				Log_info("{} {} {}.{}.{} vulkan {}.{}.{}",
						 id,
						 name,
						 VK_VERSION_MAJOR(driverVersion),
						 VK_VERSION_MINOR(driverVersion),
						 VK_VERSION_PATCH(driverVersion),
						 VK_VERSION_MAJOR(apiVersion),
						 VK_VERSION_MINOR(apiVersion),
						 VK_VERSION_PATCH(apiVersion));
				break;
			}
		}
	}

	void Device::createDevice() {
		auto indices = QueueFamilyIndices::findQueueFamilyIndices(physical_, *surface_);
		std::set<uint32_t> uniqueIndices{ *indices.graphicsFamily, *indices.presentFamily };

		std::vector<float> queuePriorities = { 1.0f };

		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
		for (auto index : uniqueIndices) {
			queueCreateInfos.emplace_back();
			queueCreateInfos.back().setQueuePriorities(queuePriorities);
			queueCreateInfos.back().setQueueFamilyIndex(index);
			queueCreateInfos.back().setQueueCount(1);
		}

		vk::DeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.setQueueCreateInfos(queueCreateInfos);
		deviceCreateInfo.setPEnabledExtensionNames(DEVICE_EXTENSIONS);
		if (VALIDATION_LAYERS_ENABLED)
			deviceCreateInfo.setPEnabledLayerNames(VALIDATION_LAYERS);

		try {
			logical_ = physical_.createDeviceUnique(deviceCreateInfo);
		}
		catch (const vk::SystemError& err) {
			Log_error("failed to create vulkan logical device. error {}", err.what());
			throw;
		}

		graphicsQueue_ = logical_->getQueue(indices.graphicsFamily.value(), 0);
		presentQueue_ = logical_->getQueue(indices.presentFamily.value(), 0);
	}

	void Device::createCommandPool() {
		auto indices = Device::QueueFamilyIndices::findQueueFamilyIndices(physical_, *surface_);

		vk::CommandPoolCreateInfo commandPoolCreateInfo{};
		commandPoolCreateInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient);
		commandPoolCreateInfo.setQueueFamilyIndex(indices.graphicsFamily.value());

		try {
			commandPool_ = logical_->createCommandPoolUnique(commandPoolCreateInfo);
		}
		catch (const vk::SystemError& err) {
			Log_error("failed to create vulkan command pool. error {}", err.what());
			throw;
		}
	}

}