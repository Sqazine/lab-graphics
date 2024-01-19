#include "VkUtils.h"
#include <stb/stb_image.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "App.h"
#include "VK/GraphicsContext.h"
namespace VkUtils
{
	Buffer::Buffer()
		: mBuffer(VK_NULL_HANDLE),
		mMemory(VK_NULL_HANDLE),
		mSize(0)
	{
	}
	Buffer::~Buffer()
	{
		Destroy();
	}

	VkResult Buffer::Create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties)
	{
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.pNext = nullptr;
		bufferCreateInfo.flags = 0;
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = usage;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferCreateInfo.queueFamilyIndexCount = 0;
		bufferCreateInfo.pQueueFamilyIndices = nullptr;

		mSize = size;

		VkResult result = vkCreateBuffer(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), &bufferCreateInfo, nullptr, &mBuffer);
		if (result == VK_SUCCESS)
		{
			VkMemoryRequirements memoryRequirements;
			vkGetBufferMemoryRequirements(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), mBuffer, &memoryRequirements);

			VkMemoryAllocateInfo memoryAllocateInfo;
			memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memoryAllocateInfo.pNext = nullptr;
			memoryAllocateInfo.allocationSize = memoryRequirements.size;
			memoryAllocateInfo.memoryTypeIndex = App::Instance().GetGraphicsContext()->GetDevice()->FindMemoryType(memoryRequirements.memoryTypeBits, memoryProperties);

			VkMemoryAllocateFlagsInfo allocationFlags = {};
			allocationFlags.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
			allocationFlags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
			if ((usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) == VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
				memoryAllocateInfo.pNext = &allocationFlags;

			result = vkAllocateMemory(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), &memoryAllocateInfo, nullptr, &mMemory);

			if (result != VK_SUCCESS)
			{
				vkDestroyBuffer(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), mBuffer, nullptr);
				mBuffer = VK_NULL_HANDLE;
				mMemory = VK_NULL_HANDLE;
			}
			else
			{
				result = vkBindBufferMemory(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), mBuffer, mMemory, 0);
				if (result != VK_SUCCESS)
				{
					vkFreeMemory(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), mMemory, nullptr);
					mBuffer = VK_NULL_HANDLE;
					mMemory = VK_NULL_HANDLE;
				}
			}
		}
		return result;
	}
	void Buffer::Destroy()
	{
		if (mBuffer)
		{
			vkDestroyBuffer(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), mBuffer, nullptr);
			mBuffer = VK_NULL_HANDLE;
		}
		if (mMemory)
		{
			vkFreeMemory(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), mMemory, nullptr);
			mMemory = VK_NULL_HANDLE;
		}
	}

	void* Buffer::Map(VkDeviceSize size, VkDeviceSize offset) const
	{
		void* mem = nullptr;
		if (size > mSize)
			size = mSize;

		VkResult result = vkMapMemory(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), mMemory, offset, size, 0, &mem);
		if (result != VK_SUCCESS)
			mem = nullptr;
		return mem;
	}
	void Buffer::Unmap() const
	{
		vkUnmapMemory(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), mMemory);
	}

	bool Buffer::UploadData(const void* data, VkDeviceSize size, VkDeviceSize offset) const
	{
		bool result = false;
		void* mem = this->Map(size, offset);
		if (mem)
		{
			std::memcpy(mem, data, size);
			this->Unmap();
		}
		return true;
	}

	VkBuffer Buffer::GetBuffer() const
	{
		return mBuffer;
	}
	VkDeviceSize Buffer::GetSize() const
	{
		return mSize;
	}

	Image::Image()
		: mFormat(VK_FORMAT_B8G8R8A8_SRGB),
		mImage(VK_NULL_HANDLE),
		mImageView(VK_NULL_HANDLE),
		mMemory(VK_NULL_HANDLE),
		mSampler(VK_NULL_HANDLE)
	{
	}
	Image::~Image()
	{
		Destroy();
	}

	VkResult Image::Create(VkImageType imageType,
		VkFormat format,
		VkExtent3D extent,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags memoryProperties)
	{
		mFormat = format;

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.pNext = nullptr;
		imageInfo.flags = 0;
		imageInfo.imageType = imageType;
		imageInfo.format = format;
		imageInfo.extent = extent;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = tiling;
		imageInfo.usage = usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.queueFamilyIndexCount = 0;
		imageInfo.pQueueFamilyIndices = nullptr;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VkResult result = vkCreateImage(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), &imageInfo, nullptr, &mImage);

		if (result == VK_SUCCESS)
		{
			VkMemoryRequirements memoryRequirements;
			vkGetImageMemoryRequirements(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), mImage, &memoryRequirements);

			VkMemoryAllocateInfo memoryAllocateInfo;
			memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memoryAllocateInfo.pNext = nullptr;
			memoryAllocateInfo.allocationSize = memoryRequirements.size;
			memoryAllocateInfo.memoryTypeIndex = App::Instance().GetGraphicsContext()->GetDevice()->FindMemoryType(memoryRequirements.memoryTypeBits, memoryProperties);

			result = vkAllocateMemory(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), &memoryAllocateInfo, nullptr, &mMemory);
			if (result != VK_SUCCESS)
			{
				vkDestroyImage(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), mImage, nullptr);
				mImage = VK_NULL_HANDLE;
				mMemory = VK_NULL_HANDLE;
			}
			else
			{
				result = vkBindImageMemory(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), mImage, mMemory, 0);
				if (result != VK_SUCCESS)
				{
					vkDestroyImage(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), mImage, nullptr);
					vkFreeMemory(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), mMemory, nullptr);
					mImage = VK_NULL_HANDLE;
					mMemory = VK_NULL_HANDLE;
				}
			}
		}
		return result;
	}
	void Image::Destroy()
	{
		if (mSampler)
		{
			vkDestroySampler(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), mSampler, nullptr);
			mSampler = VK_NULL_HANDLE;
		}
		if (mImageView)
		{
			vkDestroyImageView(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), mImageView, nullptr);
			mImageView = VK_NULL_HANDLE;
		}
		if (mImage)
		{
			vkDestroyImage(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), mImage, nullptr);
			mImage = VK_NULL_HANDLE;
		}
	}
	bool Image::Load(std::string fileName)
	{
		int width, height, channels;
		bool textureHDR = false;
		uint8_t* pixels = nullptr;

		const std::string extension = fileName.substr(fileName.length() - 3);

		if (extension == "hdr")
		{
			textureHDR = true;
			pixels = reinterpret_cast<stbi_uc*>(stbi_loadf(fileName.c_str(), &width, &height, &channels, STBI_rgb_alpha));
		}
		else
			pixels = stbi_load(fileName.c_str(), &width, &height, &channels, STBI_rgb_alpha);

		if (!pixels)
		{
			std::cout << "failed to load texture:" << fileName << std::endl;
			exit(1);
		}

		const int bpp = textureHDR ? sizeof(float[4]) : sizeof(uint8_t[4]);

		VkDeviceSize imageSize = (width * height * bpp);

		Buffer stagingBuffer;
		VkResult error = stagingBuffer.Create(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		if (error == VK_SUCCESS && stagingBuffer.UploadData(pixels, imageSize))
		{
			stbi_image_free(pixels);

			VkExtent3D imageExtent{};
			imageExtent.width = (uint32_t)width;
			imageExtent.height = (uint32_t)height;
			imageExtent.depth = 1;

			const VkFormat format = textureHDR ? VK_FORMAT_R32G32B32A32_SFLOAT : VK_FORMAT_R8G8B8A8_SRGB;

			error = this->Create(VK_IMAGE_TYPE_2D, format, imageExtent, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			if (error != VK_SUCCESS)
				return false;

			auto cmd = App::Instance().GetGraphicsContext()->GetDevice()->GetTransferCommandPool()->CreatePrimaryCommandBuffer();

			cmd->Record([&]() {

				VkImageSubresourceRange subResourceRange{};
				subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				subResourceRange.baseMipLevel = 0;
				subResourceRange.levelCount = 1;
				subResourceRange.baseArrayLayer = 0;
				subResourceRange.layerCount = 1;

				cmd->ImageBarrier(mImage, Access::NONE, Access::TRANSFER_WRITE, ImageLayout::UNDEFINED, ImageLayout::TRANSFER_DST_OPTIMAL, subResourceRange);

				VkBufferImageCopy region;
				region.bufferOffset = 0;
				region.bufferRowLength = 0;
				region.bufferImageHeight = 0;
				region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.imageSubresource.mipLevel = 0;
				region.imageSubresource.baseArrayLayer = 0;
				region.imageSubresource.layerCount = 1;
				region.imageOffset = { 0, 0, 0 };
				region.imageExtent = imageExtent;

				vkCmdCopyBufferToImage(cmd->GetHandle(), stagingBuffer.GetBuffer(), mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

				cmd->ImageBarrier(mImage, Access::TRANSFER_WRITE, Access::SHADER_READ, ImageLayout::TRANSFER_DST_OPTIMAL, ImageLayout::SHADER_READ_ONLY_OPTIMAL, subResourceRange);
				});

			cmd->Submit();
		}
		else
			stbi_image_free(pixels);

		return true;
	}
	VkResult Image::CreateImageView(VkImageViewType viewType, VkFormat format, VkImageSubresourceRange subresourceRange)
	{
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.pNext = nullptr;
		viewInfo.flags = 0;
		viewInfo.viewType = viewType;
		viewInfo.image = mImage;
		viewInfo.format = format;
		viewInfo.subresourceRange = subresourceRange;
		viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

		return vkCreateImageView(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), &viewInfo, nullptr, &mImageView);
	}
	VkResult Image::CreateSampler(VkFilter magFilter, VkFilter minFilter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode)
	{
		VkSamplerCreateInfo samplerCreateInfo;
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.pNext = nullptr;
		samplerCreateInfo.flags = 0;
		samplerCreateInfo.magFilter = magFilter;
		samplerCreateInfo.minFilter = minFilter;
		samplerCreateInfo.addressModeU = addressMode;
		samplerCreateInfo.addressModeV = addressMode;
		samplerCreateInfo.addressModeW = addressMode;
		samplerCreateInfo.mipLodBias = 0;
		samplerCreateInfo.anisotropyEnable = VK_FALSE;
		samplerCreateInfo.maxAnisotropy = 1;
		samplerCreateInfo.compareEnable = VK_FALSE;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerCreateInfo.minLod = 0;
		samplerCreateInfo.maxLod = 0;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
		samplerCreateInfo.mipmapMode = mipmapMode;

		return  vkCreateSampler(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), &samplerCreateInfo, nullptr, &mSampler);
	}

	VkFormat Image::GetFormat() const
	{
		return mFormat;
	}
	VkImage Image::GetImage() const
	{
		return mImage;
	}
	VkImageView Image::GetImageView() const
	{
		return mImageView;
	}
	VkSampler Image::GetSampler() const
	{
		return mSampler;
	}

	Shader::Shader()
		: mModule(VK_NULL_HANDLE)
	{
	}
	Shader::~Shader()
	{
		this->Destroy();
	}
	bool Shader::LoadFromFile(std::string_view fileName)
	{
		std::ifstream file(fileName.data(), std::ios::in | std::ios::binary);
		if (!file.is_open())
		{
			std::cout << "Failed to load shader file:" << fileName << std::endl;
			return false;
		}

		std::stringstream sstream;
		sstream << file.rdbuf();

		std::string content = sstream.str();

		VkShaderModuleCreateInfo shaderCreateInfo{};
		shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderCreateInfo.pNext = nullptr;
		shaderCreateInfo.flags = 0;
		shaderCreateInfo.pCode = reinterpret_cast<uint32_t*>(content.data());
		shaderCreateInfo.codeSize = content.size();

		VkResult result = vkCreateShaderModule(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), &shaderCreateInfo, nullptr, &mModule);

		return result == VK_SUCCESS;
	}
	void Shader::Destroy()
	{
		if (mModule)
		{
			vkDestroyShaderModule(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), mModule, nullptr);
			mModule = VK_NULL_HANDLE;
		}
	}

	VkPipelineShaderStageCreateInfo Shader::GetShaderStage(VkShaderStageFlagBits stage)
	{
		VkPipelineShaderStageCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		info.pNext = nullptr;
		info.flags = 0;
		info.stage = stage;
		info.module = mModule;
		info.pName = "main";
		info.pSpecializationInfo = nullptr;

		return info;
	}

	VkDeviceOrHostAddressKHR GetBufferDeviceAddress(const Buffer& buffer)
	{
		VkBufferDeviceAddressInfoKHR info = {};
		info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		info.pNext = nullptr;
		info.buffer = buffer.GetBuffer();

		VkDeviceOrHostAddressKHR result;
		result.deviceAddress = App::Instance().GetGraphicsContext()->GetDevice()->vkGetBufferDeviceAddressKHR(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), &info);
		return result;
	}
	VkDeviceOrHostAddressConstKHR GetBufferDeviceAddressConst(const Buffer& buffer)
	{
		VkDeviceOrHostAddressKHR address = GetBufferDeviceAddress(buffer);

		VkDeviceOrHostAddressConstKHR result;
		result.deviceAddress = address.deviceAddress;
		return result;
	}
}