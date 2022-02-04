#pragma once

#include <vulkan/vulkan.hpp>

#include <vector>

#include "Log.hpp"

namespace fve {

	class Device;

	class Buffer final {
	public:
		explicit Buffer(Device& device,
						vk::DeviceSize instanceSize,
						vk::DeviceSize instanceCount,
						vk::BufferUsageFlags usageFlags,
						vk::MemoryPropertyFlags memoryPropertyFlags);

		~Buffer() noexcept;

		bool map() noexcept;
		bool unmap() noexcept;

		template<typename T>
		bool write(const std::vector<T>& data) {
			const size_t size = data.size() * sizeof(T);

			if (size != bufferSize_) {
				Log_error("failed to write data to the vulkan buffer. size mismatch");
				return false;
			}

			if (!mapped_ && !map()) {
				Log_error("failed to write data to the vulkan buffer. failed to map buffer memory");
				return false;
			}

			std::memcpy(mapped_, data.data(), bufferSize_);

			return true;
		}

		inline vk::Buffer buffer() const noexcept { return buffer_.first; }
		inline vk::DeviceSize bufferSize() const noexcept { return bufferSize_; }
		inline vk::DeviceSize instanceCount() const noexcept { return instanceCount_; }

	private:
		Device& device_;
		vk::DeviceSize bufferSize_;
		vk::DeviceSize instanceSize_;
		vk::DeviceSize instanceCount_;
		vk::BufferUsageFlags usageFlags_;
		std::pair<vk::Buffer, vk::DeviceMemory> buffer_;
		void* mapped_ = nullptr;

	};

}