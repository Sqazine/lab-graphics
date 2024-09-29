#include "Sampler.h"
#include "Device.h"
#include "Utils.h"

Sampler::Sampler(const Device &device)
    : mDevice(device), mHandle(VK_NULL_HANDLE)
{
}

Sampler::~Sampler()
{
    if (mHandle)
        vkDestroySampler(mDevice.GetHandle(), mHandle, nullptr);
}

const VkSampler &Sampler::GetHandle()
{
    if (mHandle == VK_NULL_HANDLE || mIsDirty)
        Build();
    return mHandle;
}

Sampler &Sampler::SetMagFilter(FilterMode filter)
{
    SET(mMagFilter, filter);
}
Sampler &Sampler::SetMinFilter(FilterMode filter)
{
    SET(mMinFilter, filter);
}
Sampler &Sampler::SetAddressU(AddressMode address)
{
    SET(mAddressU, address);
}
Sampler &Sampler::SetAddressV(AddressMode address)
{
    SET(mAddressV, address);
}
Sampler &Sampler::SetAddressW(AddressMode address)
{
    SET(mAddressW, address);
}
Sampler &Sampler::SetAnisotropyLevel(float level)
{
    SET(mMaxAnisotropyLevel, level);
}
Sampler &Sampler::SetBorderColor(BorderColor borderColor)
{
    SET(mBorderColor, borderColor);
}
Sampler &Sampler::SetMipMapMode(MipMapMode mipmapMode)
{
    SET(mMipMapMode, mipmapMode);
}
Sampler &Sampler::SetMipMapBias(float bias)
{
    SET(mMipMapBias, bias);
}
Sampler &Sampler::SetMinMipMapLevel(float level)
{
    SET(mMinMipMapLevel, level);
}
Sampler &Sampler::SetMaxMipMapLevel(float level)
{
    SET(mMaxMipMapLevel, level);
}

const FilterMode &Sampler::GetMagFilter() const
{
    return mMagFilter;
}
const FilterMode &Sampler::GetMinFilter() const
{
    return mMinFilter;
}
const AddressMode &Sampler::GetAddressModeU() const
{
    return mAddressU;
}
const AddressMode &Sampler::GetAddressModeV() const
{
    return mAddressV;
}
const AddressMode &Sampler::GetAddressModeW() const
{
    return mAddressW;
}

float Sampler::GetMaxAnisotropyLevel() const
{
    return mMaxAnisotropyLevel;
}

const BorderColor &Sampler::GetBorderColor() const
{
    return mBorderColor;
}

const MipMapMode &Sampler::GetMipMapMode() const
{
    return mMipMapMode;
}

float Sampler::GetMipMapBias() const
{
    return mMipMapBias;
}
float Sampler::GetMinMipMapLevel() const
{
    return mMinMipMapLevel;
}
float Sampler::GetMaxMipMapLevel() const
{
    return mMaxMipMapLevel;
}

void Sampler::Build()
{
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = FILTER_MODE_CAST(mMagFilter);
    info.minFilter = FILTER_MODE_CAST(mMinFilter);
    info.addressModeU = ADDRESS_MODE_CAST(mAddressU);
    info.addressModeV = ADDRESS_MODE_CAST(mAddressV);
    info.addressModeW = ADDRESS_MODE_CAST(mAddressW);
    info.anisotropyEnable = mMaxAnisotropyLevel > 0 ? VK_TRUE : VK_FALSE;
    info.maxAnisotropy = mMaxAnisotropyLevel;
    info.borderColor = BORDER_COLOR_CAST(mBorderColor);
    info.unnormalizedCoordinates = VK_FALSE;
    info.compareEnable = VK_FALSE;
    info.compareOp = VK_COMPARE_OP_ALWAYS;
    info.mipmapMode = MIPMAP_MODE_CAST(mMipMapMode);
    info.mipLodBias = mMipMapBias;
    info.minLod = mMinMipMapLevel;
    info.maxLod = mMaxMipMapLevel;

    VK_CHECK(vkCreateSampler(mDevice.GetHandle(), &info, nullptr, &mHandle));
    mIsDirty = false;
}
