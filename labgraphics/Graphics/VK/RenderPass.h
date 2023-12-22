#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "Format.h"
class RenderPass
{
public:
    RenderPass(const class Device &device, const std::vector<Format> &colorformats, Format depthStencilFormat=Format::UNDEFINED);
    RenderPass(const class Device &device, const Format &colorformat, Format depthStencilFormat=Format::UNDEFINED);
    ~RenderPass();

    const VkRenderPass &GetHandle() const;

private:
    const class Device &mDevice;
    VkRenderPass mHandle;
};