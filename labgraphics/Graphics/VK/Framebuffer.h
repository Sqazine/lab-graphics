#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <map>
#include "RenderPass.h"
#include "ImageView.h"

class Framebuffer
{
public:
    Framebuffer(const class Device &device);
    ~Framebuffer();

    const VkFramebuffer &GetHandle();

    Framebuffer &AttachRenderPass(const RenderPass *renderPass);
    Framebuffer &BindAttachment(uint32_t slot, const ImageView2D *attachment);
    Framebuffer &SetExtent(uint32_t w, uint32_t h);

private:
    const class Device &mDevice;

    void Build();

    bool mIsDirty{true};

    std::map<uint32_t, VkImageView> mAttachmentCache;

    VkFramebufferCreateInfo mInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};

    VkFramebuffer mHandle;
};