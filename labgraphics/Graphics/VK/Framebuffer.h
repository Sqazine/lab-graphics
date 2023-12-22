#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "RenderPass.h"
#include "ImageView.h"

class Framebuffer
{
public:
    Framebuffer(const class Device &device, const RenderPass *renderPass, const std::vector<ImageView2D *> &attachments, uint32_t width, uint32_t height);
    ~Framebuffer();
    const VkFramebuffer &GetHandle() const;

private:
    const class Device &mDevice;

    VkFramebuffer mHandle;
};