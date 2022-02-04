#include "Mesh.hpp"
#include "Device.hpp"

namespace fve {

	Mesh::Mesh(Device& device, const std::vector<Vertex>& vertices, const std::vector<Index>& indices) : device_{ device } {
		vertexBuffer_ = createBuffer(vk::BufferUsageFlagBits::eVertexBuffer, vertices);
		if (!indices.empty())
			indexBuffer_ = createBuffer(vk::BufferUsageFlagBits::eIndexBuffer, indices);
	}

	void Mesh::bind(vk::CommandBuffer commandBuffer) {
		std::vector<vk::Buffer> buffers{ vertexBuffer_->buffer() };
		std::vector<vk::DeviceSize> offsets{ 0 };

		commandBuffer.bindVertexBuffers(0, buffers, offsets);
		if (indexBuffer_)
			commandBuffer.bindIndexBuffer(indexBuffer_->buffer(), 0, vk::IndexType::eUint32);
	}

	void Mesh::draw(vk::CommandBuffer commandBuffer) {
		if (indexBuffer_) 
			commandBuffer.drawIndexed(indexBuffer_->instanceCount(), 1, 0, 0, 0);
		else 
			commandBuffer.draw(vertexBuffer_->instanceCount(), 1, 0, 0);
	}

}