#pragma once
#include <cassert>
#include <vulkan/vulkan.h>
#include <string_view>
#include <memory>
#include "labgraphics.h"
namespace VkUtils
{

    namespace Details
    {
        static Device* sDevice;
        static VkCommandPool sCommandPool;
        static const TransferQueue* sTransferQueue;
        static VkPhysicalDeviceMemoryProperties sPhysicalDeviceMemoryProperties;
    }

    void Init(Device* device, VkCommandPool commandPool);
    uint32_t GetMemoryType(VkMemoryRequirements &memoryRequirimemts, VkMemoryPropertyFlags memoryProperties);
    void ImageBarrier(VkCommandBuffer commandBuffer,
                      VkImage image,
                      VkImageSubresourceRange &subresourceRange,
                      VkAccessFlags srcAccessMask,
                      VkAccessFlags dstAccessMask,
                      VkImageLayout oldLayout,
                      VkImageLayout newLayout);
    class Buffer
    {
    public:
        Buffer();
        ~Buffer();

        VkResult Create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties);
        void Destroy();

        void *Map(VkDeviceSize size = UINT64_MAX, VkDeviceSize offset = 0) const;
        void Unmap() const;

        bool UploadData(const void *data, VkDeviceSize size, VkDeviceSize offset = 0) const;

        VkBuffer GetBuffer() const;
        VkDeviceSize GetSize() const;

    private:
        VkBuffer mBuffer;
        VkDeviceMemory mMemory;
        VkDeviceSize mSize;
    };

    class Image
    {
    public:
        Image();
        ~Image();

        VkResult Create(VkImageType imageType, VkFormat format, VkExtent3D extent, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryProperties);
        void Destroy();
        bool Load(std::string fileName);
        VkResult CreateImageView(VkImageViewType viewType, VkFormat format, VkImageSubresourceRange subresourceRange);
        VkResult CreateSampler(VkFilter magFilter, VkFilter minFilter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode);

        VkFormat GetFormat() const;
        VkImage GetImage() const;
        VkImageView GetImageView() const;
        VkSampler GetSampler() const;

    private:
        VkFormat mFormat;
        VkImage mImage;
        VkDeviceMemory mMemory;
        VkImageView mImageView;
        VkSampler mSampler;
    };

    class Shader
    {
    public:
        Shader();
        ~Shader();
        bool LoadFromFile(std::string_view fileName);
        void Destroy();

        VkPipelineShaderStageCreateInfo GetShaderStage(VkShaderStageFlagBits stage);

    private:
        VkShaderModule mModule;
    };

    VkDeviceOrHostAddressKHR GetBufferDeviceAddress(const Buffer &buffer);
    VkDeviceOrHostAddressConstKHR GetBufferDeviceAddressConst(const Buffer &buffer);
}