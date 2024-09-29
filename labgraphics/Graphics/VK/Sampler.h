#pragma once
#include <cstdint>
#include <type_traits>
#include <vulkan/vulkan.h>
#include "Enum.h"
class Sampler
{
public:
    Sampler(const class Device &device);
    ~Sampler();

    const VkSampler &GetHandle();

    Sampler &SetMagFilter(FilterMode filter);
    Sampler &SetMinFilter(FilterMode filter);
    Sampler &SetAddressU(AddressMode address);
    Sampler &SetAddressV(AddressMode address);
    Sampler &SetAddressW(AddressMode address);
    Sampler &SetAnisotropyLevel(float level);
    Sampler &SetBorderColor(BorderColor borderColor);
    Sampler &SetMipMapMode(MipMapMode mipmapMode);
    Sampler &SetMipMapBias(float bias);
    Sampler &SetMinMipMapLevel(float level);
    Sampler &SetMaxMipMapLevel(float level);

    const FilterMode &GetMagFilter() const;
    const FilterMode &GetMinFilter() const;
    const AddressMode &GetAddressModeU() const;
    const AddressMode &GetAddressModeV() const;
    const AddressMode &GetAddressModeW() const;

    float GetMaxAnisotropyLevel() const;

    const BorderColor &GetBorderColor() const;

    const MipMapMode &GetMipMapMode() const;

    float GetMipMapBias() const;
    float GetMinMipMapLevel() const;
    float GetMaxMipMapLevel() const;

private:
    void Build();

    const class Device &mDevice;

    VkSampler mHandle;

    bool mIsDirty{true};

    FilterMode mMagFilter{FilterMode::LINEAR};
    FilterMode mMinFilter{FilterMode::LINEAR};
    AddressMode mAddressU{AddressMode::REPEAT};
    AddressMode mAddressV{AddressMode::REPEAT};
    AddressMode mAddressW{AddressMode::REPEAT};

    float mMaxAnisotropyLevel{0};

    BorderColor mBorderColor{BorderColor::FLOAT_OPAQUE_BLACK};

    MipMapMode mMipMapMode{MipMapMode::LINEAR};
    float mMipMapBias{0.0f};
    float mMinMipMapLevel{0.0f};
    float mMaxMipMapLevel{0.0f};
};