#pragma once

#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <memory>

#include "Buffer.hpp"

namespace fve {

	class Mesh final {
	public:
		using Index = uint32_t;

		struct Vertex {
			Vertex() = default;
			Vertex(glm::vec3 p) : position{ std::move(p) } {
			}

			glm::vec3 position;

			inline static std::vector<vk::VertexInputBindingDescription> bindingDescriptions() noexcept {
				std::vector<vk::VertexInputBindingDescription> bindingDescriptions{};
				bindingDescriptions.push_back({ 0, sizeof(Vertex), vk::VertexInputRate::eVertex });
				return bindingDescriptions;
			}

			inline static std::vector<vk::VertexInputAttributeDescription> attributeDescriptions() noexcept {
				std::vector<vk::VertexInputAttributeDescription> attributeDescriptions{};
				attributeDescriptions.push_back({ 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position) });
				return attributeDescriptions;
			}

			inline bool operator==(const Vertex& other) const noexcept {
				return position == other.position;
			}

		};

		explicit Mesh(Device& device, const std::vector<Vertex>& vertices, const std::vector<Index>& indices = {});

		Mesh(const Mesh&) = delete;
		Mesh& operator=(const Mesh&) = delete;

		~Mesh() noexcept = default;

		void bind(vk::CommandBuffer commandBuffer);
		void draw(vk::CommandBuffer commandBuffer);

	private:
		template<typename T>
		std::unique_ptr<Buffer> createBuffer(vk::BufferUsageFlags usageFlags, const std::vector<T>& data) const {
			Buffer stagingBuffer{
				device_,
				sizeof(data[0]),
				data.size(),
				vk::BufferUsageFlagBits::eTransferSrc,
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
			};

			stagingBuffer.map();
			stagingBuffer.write(data);

			auto buffer = std::make_unique<Buffer>(device_,
												   sizeof(data[0]),
												   data.size(),
												   usageFlags | vk::BufferUsageFlagBits::eTransferDst,
												   vk::MemoryPropertyFlagBits::eDeviceLocal);

			device_.copyBuffer(stagingBuffer.buffer(), buffer->buffer(), stagingBuffer.bufferSize());

			return buffer;
		}

		Device& device_;
		std::unique_ptr<Buffer> indexBuffer_ = nullptr;
		std::unique_ptr<Buffer> vertexBuffer_ = nullptr;

	};



}