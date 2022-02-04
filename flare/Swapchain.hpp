#pragma once

#include <vulkan/vulkan.hpp>

namespace fve {

	class Device;

	class Swapchain final {
	public:
		struct SupportDetails {
			inline static SupportDetails findSwapchainSupportDetails(vk::PhysicalDevice device, vk::SurfaceKHR surface) noexcept {
				SupportDetails details{};
				details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
				details.formats = device.getSurfaceFormatsKHR(surface);
				details.presentModes = device.getSurfacePresentModesKHR(surface);
				return details;
			}

			vk::SurfaceCapabilitiesKHR capabilities;
			std::vector<vk::SurfaceFormatKHR> formats;
			std::vector<vk::PresentModeKHR> presentModes;
		};

		static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

		explicit Swapchain(Device& device, vk::Extent2D windowExtent);

		~Swapchain() noexcept;

		Swapchain(const Swapchain&) = delete;
		Swapchain& operator=(const Swapchain&) = delete;

		inline uint32_t size() const noexcept { return images_.size(); }
		inline vk::RenderPass renderPass() const noexcept { return *renderPass_; };
		inline vk::Framebuffer framebuffer(size_t index) const { return *framebuffers_[index]; };
		inline vk::Extent2D extent() const noexcept { return extent_; }
		inline vk::Format imageFormat() const noexcept { return imageFormat_; };

		[[nodiscard]] vk::Result acquireNextImage(uint32_t& imageIndex);
		[[nodiscard]] vk::Result submit(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

	private:
		vk::SurfaceFormatKHR pickSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats) const noexcept;
		vk::PresentModeKHR pickPresentMode(const std::vector<vk::PresentModeKHR> presentModes) const noexcept;
		vk::Extent2D pickExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const noexcept;

		void createSwapchain();
		void createImageViews();
		void createRenderPass();
		void createFramebuffers();
		void createSynchronization();

		Device& device_;
		vk::Extent2D extent_;
		vk::Extent2D windowExtent_;
		vk::UniqueSwapchainKHR swapchain_;
		vk::Format imageFormat_;
		std::vector<vk::Image> images_;
		std::vector<vk::ImageView> imageViews_;
		vk::UniqueRenderPass renderPass_;
		std::vector<vk::UniqueFramebuffer> framebuffers_;
		std::vector<vk::UniqueSemaphore> imageAvailableSemaphores_;
		std::vector<vk::UniqueSemaphore> renderFinishedSemaphores_;
		std::vector<vk::UniqueFence> inFlightFences_;
		size_t currentFrame_ = 0;
	};

}