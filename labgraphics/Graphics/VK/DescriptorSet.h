#pragma once
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <cstdint>
#include "Buffer.h"
#include "ImageView.h"
#include "Image.h"
#include "AS.h"
#include "Sampler.h"

struct DescriptorImageInfo
{
    VkSampler sampler;
    const ImageView2D *imageView;
    ImageLayout imageLayout;

    VkDescriptorImageInfo ToVkDescriptorImageInfo() const
    {
        VkDescriptorImageInfo result;
        result.sampler=sampler;
        result.imageView=imageView->GetHandle();
        result.imageLayout=IMAGE_LAYOUT_CAST(imageLayout);
        return result;
    }
};

class DescriptorSet
{
public:
    DescriptorSet(const class Device &device, class DescriptorPool *descPool, class DescriptorSetLayout *descLayout);
    ~DescriptorSet();

    const VkDescriptorSet &GetHandle() const;

    DescriptorSet &WriteBuffer(uint32_t binding, const Buffer *buffer, uint64_t offset, uint64_t size);
    DescriptorSet &WriteBuffer(uint32_t binding, const Buffer *buffer);
    DescriptorSet &WriteImage(uint32_t binding, const ImageView2D *imgView, ImageLayout layout,Sampler *sampler=nullptr);
    DescriptorSet &WriteImageArray(uint32_t binding, const std::vector<DescriptorImageInfo>& imageInfos);
    DescriptorSet &WriteAccelerationStructure(uint32_t binding, const VkAccelerationStructureKHR &as);

    void Update();

private:
    const class Device &mDevice;

    std::unordered_map<uint32_t, VkDescriptorBufferInfo> mBufferInfoCache;
    std::unordered_map<uint32_t, VkDescriptorImageInfo> mImageInfoCache;
    std::unordered_map<uint32_t, std::vector<VkDescriptorImageInfo>> mImagesInfoCache;
    std::unordered_map<uint32_t, VkWriteDescriptorSetAccelerationStructureKHR> mASInfoCache;

    class DescriptorPool *mDescriptorPool;
    DescriptorSetLayout *mDescriptorLayout;
    VkDescriptorSet mHandle;
};