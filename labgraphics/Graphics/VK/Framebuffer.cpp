#include "Framebuffer.h"
#include "Utils.h"
#include "Device.h"
#include <iostream>

Framebuffer::Framebuffer(const Device &device)
	: mDevice(device), mHandle(nullptr)
{
}

Framebuffer::~Framebuffer()
{
	if (mHandle)
		vkDestroyFramebuffer(mDevice.GetHandle(), mHandle, nullptr);
}

const VkFramebuffer &Framebuffer::GetHandle()
{
	Build();
	return mHandle;
}

Framebuffer &Framebuffer::AttachRenderPass(const RenderPass *renderPass)
{
	SET(mInfo.renderPass, renderPass->GetHandle());
}

Framebuffer &Framebuffer::BindAttachment(uint32_t slot, const ImageView2D *attachment)
{
	SET(mAttachmentCache[slot], attachment->GetHandle());
}

Framebuffer &Framebuffer::SetExtent(uint32_t w, uint32_t h)
{
	if (mInfo.width != w)
	{
		mInfo.width = w;

		mIsDirty = true;
	}

	if (mInfo.height != h)
	{
		mInfo.height = h;
		mIsDirty = true;
	}
	return *this;
}

void Framebuffer::Build()
{
	if (mIsDirty)
	{
		std::vector<VkImageView> attachments;
		for (auto &[k, v] : mAttachmentCache)
			attachments.emplace_back(v);

		mInfo.attachmentCount = attachments.size();
		mInfo.pAttachments = attachments.data();
		mInfo.layers=1;
		VK_CHECK(vkCreateFramebuffer(mDevice.GetHandle(), &mInfo, nullptr, &mHandle));

		mIsDirty = false;
	}
}