#include <memory>
#include <vector>
#include "Buffer.h"
template <typename T>
inline std::unique_ptr<Buffer> Device::CreateRasterVertexBuffer(const std::vector<T> &vertices)
{
	return std::move(std::make_unique<VertexBuffer>(const_cast<Device &>(*this), vertices));
}

template <typename T>
inline std::unique_ptr<Buffer> Device::CreateRayTraceVertexBuffer(const std::vector<T> &vertices) const
{
	return std::move(std::make_unique<VertexBuffer>(const_cast<Device &>(*this), vertices, BufferUsage::ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY));
}

template <typename T>
inline std::unique_ptr<IndexBuffer> Device::CreateRasterIndexBuffer(const std::vector<T> &indices) const
{
	return std::move(std::make_unique<IndexBuffer>(const_cast<Device &>(*this), indices));
}

template <typename T>
inline std::unique_ptr<IndexBuffer> Device::CreateRayTraceIndexBuffer(const std::vector<T> &indices) const
{
	return std::move(std::make_unique<IndexBuffer>(const_cast<Device &>(*this), indices, BufferUsage::ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY));
}

template <typename T>
inline std::unique_ptr<UniformBuffer<T>> Device::CreateUniformBuffer() const
{
	return std::move(std::make_unique<UniformBuffer<T>>(const_cast<Device &>(*this)));
}