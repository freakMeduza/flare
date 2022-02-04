#include "Swapchain.hpp"
#include "Device.hpp"
#include "Log.hpp"

namespace fve {

	Swapchain::Swapchain(Device& device, vk::Extent2D windowExtent) : device_{ device }, windowExtent_{ windowExtent } {
		createSwapchain();
		createImageViews();
		createRenderPass();
		createFramebuffers();
		createSynchronization();
	}

	Swapchain::~Swapchain() noexcept {
		for (auto view : imageViews_)
			device_.logical().destroyImageView(view);
	}

	vk::Result Swapchain::acquireNextImage(uint32_t& imageIndex) {
		if (device_.logical().waitForFences(1, &(*inFlightFences_[currentFrame_]), VK_TRUE, std::numeric_limits<uint64_t>::max()) != vk::Result::eSuccess)
			throw std::runtime_error{ "failed to wait for fence" };
		if (device_.logical().resetFences(1, &(*inFlightFences_[currentFrame_])) != vk::Result::eSuccess)
			throw std::runtime_error{ "failed to reset fence" };
		auto rv = device_.logical().acquireNextImageKHR(*swapchain_, std::numeric_limits<uint64_t>::max(), *imageAvailableSemaphores_[currentFrame_], nullptr);
		if (rv.result == vk::Result::eSuccess)
			imageIndex = rv.value;
		return rv.result;
	}

	vk::Result Swapchain::submit(vk::CommandBuffer commandBuffer, uint32_t imageIndex) {
		std::array<uint32_t, 1> imageIndices{ imageIndex };
		std::array<vk::CommandBuffer, 1> commandBuffers{ commandBuffer };
		std::array<vk::SwapchainKHR, 1> swapchains{ *swapchain_ };
		std::array<vk::Semaphore, 1> waitSemaphores{ *imageAvailableSemaphores_[currentFrame_] };
		std::array<vk::Semaphore, 1> signalSemaphores{ *renderFinishedSemaphores_[currentFrame_] };
		std::array<vk::PipelineStageFlags, 1> waitStages{ vk::PipelineStageFlagBits::eColorAttachmentOutput };

		vk::SubmitInfo submitInfo{};
		submitInfo.setSignalSemaphores(signalSemaphores);
		submitInfo.setWaitSemaphores(waitSemaphores);
		submitInfo.setWaitDstStageMask(waitStages);
		submitInfo.setCommandBuffers(commandBuffers);

		try {
			device_.graphicsQueue().submit(submitInfo, *inFlightFences_[currentFrame_]);
		}
		catch (const vk::SystemError& err) {
			Log_error("failed to submit command buffer. error {}", err.what());
			throw;
		}

		vk::PresentInfoKHR presentInfo{};
		presentInfo.setWaitSemaphores(signalSemaphores);
		presentInfo.setSwapchains(swapchains);
		presentInfo.setImageIndices(imageIndices);

		auto res = device_.presentQueue().presentKHR(presentInfo);

		currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;

		return res;
	}

	vk::SurfaceFormatKHR Swapchain::pickSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats) const noexcept {
		for (const auto& format : formats) {
			if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
				return format;
		}
		return formats[0];
	}

	vk::PresentModeKHR Swapchain::pickPresentMode(const std::vector<vk::PresentModeKHR> presentModes) const noexcept {
		for (const auto& mode : presentModes) {
			if (mode == vk::PresentModeKHR::eMailbox)
				return mode;
		}
		return vk::PresentModeKHR::eFifo;
	}

	vk::Extent2D Swapchain::pickExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const noexcept {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			return capabilities.currentExtent;
		else {
			vk::Extent2D actualExtent = windowExtent_;
			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
			return actualExtent;
		}
	}

	void Swapchain::createSwapchain() {
		auto details = SupportDetails::findSwapchainSupportDetails(device_.physical(), device_.surface());
		auto surfaceFormat = pickSurfaceFormat(details.formats);
		auto presentMode = pickPresentMode(details.presentModes);
		auto extent = pickExtent(details.capabilities);

		uint32_t imageCount = details.capabilities.minImageCount + 1;
		if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount)
			imageCount = details.capabilities.maxImageCount;

		vk::SwapchainCreateInfoKHR swapchainCreateInfo{};
		swapchainCreateInfo.setSurface(device_.surface());
		swapchainCreateInfo.setMinImageCount(imageCount);
		swapchainCreateInfo.setImageFormat(surfaceFormat.format);
		swapchainCreateInfo.setImageColorSpace(surfaceFormat.colorSpace);
		swapchainCreateInfo.setImageExtent(extent);
		swapchainCreateInfo.setImageArrayLayers(1);
		swapchainCreateInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);

		auto indices = Device::QueueFamilyIndices::findQueueFamilyIndices(device_.physical(), device_.surface());

		std::array<uint32_t, 2> queueFamilyIndices{ *indices.graphicsFamily, *indices.presentFamily };
		if (indices.graphicsFamily != indices.presentFamily) {
			swapchainCreateInfo.setImageSharingMode(vk::SharingMode::eConcurrent);
			swapchainCreateInfo.setQueueFamilyIndices(queueFamilyIndices);
		}
		else {
			swapchainCreateInfo.setImageSharingMode(vk::SharingMode::eExclusive);
		}

		swapchainCreateInfo.setPreTransform(details.capabilities.currentTransform);
		swapchainCreateInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
		swapchainCreateInfo.setPresentMode(presentMode);
		swapchainCreateInfo.setClipped(true);
		swapchainCreateInfo.setOldSwapchain(nullptr);

		try {
			swapchain_ = device_.logical().createSwapchainKHRUnique(swapchainCreateInfo);
		}
		catch (const vk::SystemError& err) {
			Log_error("failed to create swapchain. error {}", err.what());
			throw;
		}

		extent_ = extent;
		imageFormat_ = surfaceFormat.format;
		images_ = device_.logical().getSwapchainImagesKHR(*swapchain_);
	}

	void Swapchain::createImageViews() {
		imageViews_.resize(images_.size());

		for (auto i = 0u; i < imageViews_.size(); ++i) {
			vk::ImageViewCreateInfo imageViewCreateInfo{};
			imageViewCreateInfo.image = images_[i];
			imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
			imageViewCreateInfo.format = imageFormat_;
			imageViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
			imageViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
			imageViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
			imageViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
			imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 1;

			try {
				imageViews_[i] = device_.logical().createImageView(imageViewCreateInfo);
			}
			catch (const vk::SystemError& err) {
				Log_error("failed to create vulkan image view. error {}", err.what());
				throw;
			}
		}
	}

	void Swapchain::createRenderPass() {
		vk::AttachmentDescription colorAttachment = {};
		colorAttachment.format = imageFormat_;
		colorAttachment.samples = vk::SampleCountFlagBits::e1;
		colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
		colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
		colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
		colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

		vk::AttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

		vk::SubpassDescription subpass = {};
		subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		vk::RenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		try {
			renderPass_ = device_.logical().createRenderPassUnique(renderPassInfo);
		}
		catch (vk::SystemError err) {
			throw std::runtime_error("failed to create render pass!");
		}
	}

	void Swapchain::createFramebuffers() {
		framebuffers_.resize(imageViews_.size());

		for (size_t i = 0; i < framebuffers_.size(); i++) {
			vk::ImageView attachments[] = {
				imageViews_[i]
			};

			vk::FramebufferCreateInfo framebufferCreateInfo{};
			framebufferCreateInfo.renderPass = *renderPass_;
			framebufferCreateInfo.attachmentCount = 1;
			framebufferCreateInfo.pAttachments = attachments;
			framebufferCreateInfo.width = extent_.width;
			framebufferCreateInfo.height = extent_.height;
			framebufferCreateInfo.layers = 1;

			try {
				framebuffers_[i] = device_.logical().createFramebufferUnique(framebufferCreateInfo);
			}
			catch (const vk::SystemError& err) {
				Log_error("failed to create vulkan framebuffer. error {}", err.what());
				throw;
			}
		}
	}

	void Swapchain::createSynchronization() {
		imageAvailableSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);

		try {
			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
				imageAvailableSemaphores_[i] = device_.logical().createSemaphoreUnique({});
				renderFinishedSemaphores_[i] = device_.logical().createSemaphoreUnique({});
				inFlightFences_[i] = device_.logical().createFenceUnique({ vk::FenceCreateFlagBits::eSignaled });
			}
		}
		catch (const vk::SystemError& err) {
			Log_error("failed to create synchronization objects for a frame. error {}", err.what());
			throw;
		}
	}


}