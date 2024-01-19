#include "Texture.h"
#include "VK/Buffer.h"
#include "VK/CommandBuffer.h"
#include "VK/CommandPool.h"

Texture::Texture(Device &device,
				 ImageData *texture,
				 Format format,
				 ImageTiling tiling)
{
	auto stagingBuffer = device.CreateCPUBuffer(texture->GetPixels<void>(), texture->GetImageSize(), BufferUsage::TRANSFER_SRC);

	const auto extent = VkExtent2D{static_cast<uint32_t>(texture->GetWidth()), static_cast<uint32_t>(texture->GetHeight())};

	mImage = std::make_unique<GpuImage2D>(device, extent.width, extent.height, format,tiling, ImageUsage::TRANSFER_DST | ImageUsage::SAMPLED);

	mImage->UploadDataFrom(texture->GetImageSize(), stagingBuffer.get(), ImageLayout::UNDEFINED, ImageLayout::SHADER_READ_ONLY_OPTIMAL);

	mSampler.reset(new Sampler(device));
}