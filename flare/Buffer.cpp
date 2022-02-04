#include "Buffer.hpp"
#include "Device.hpp"

#include <stdexcept>

namespace fve {

	Buffer::Buffer(Device& device,
				   vk::DeviceSize instanceSize,
				   vk::DeviceSize instanceCount,
				   vk::BufferUsageFlags usageFlags,
				   vk::MemoryPropertyFlags memoryPropertyFlags)
		:
		device_{ device },
		bufferSize_{ instanceSize * instanceCount },
		instanceSize_{ instanceSize },
		instanceCount_{ instanceCount },
		usageFlags_{ usageFlags }
	{
		buffer_ = device_.createBuffer(instanceSize, instanceCount, usageFlags, memoryPropertyFlags);
	}

	Buffer::~Buffer() noexcept {
		device_.logical().destroyBuffer(buffer_.first);
		device_.logical().freeMemory(buffer_.second);
	}

	bool Buffer::map() noexcept {
		try {
			mapped_ = device_.logical().mapMemory(buffer_.second, 0, bufferSize_);
		}
		catch (const vk::SystemError& err) {
			Log_error("failed to map buffer memory. error {}", err.what());
		}
		catch (const std::exception& ex) {
			Log_error("failed to map buffer memory. error {}", ex.what());
		}
		catch (...) {
			Log_error("failed to map buffer memory. unknown exception");
		}
		return false;
	}

	bool Buffer::unmap() noexcept {
		if (!mapped_)
			return false;
		device_.logical().unmapMemory(buffer_.second);
		mapped_ = nullptr;
		return true;
	}

}