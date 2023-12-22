#include "Framebuffer.h"
#include "Utils.h"
#include "Device.h"
#include <iostream>

Framebuffer::Framebuffer(const Device &device, const RenderPass *renderPass, const std::vector<ImageView2D *> &attachments, uint32_t width, uint32_t height)
	: mDevice(device)
{
	std::vector<VkImageView> rawAttachments(attachments.size());
	for (int32_t i = 0; i < attachments.size(); ++i)
		rawAttachments[i] = attachments[i]->GetHandle();

	VkFramebufferCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.pNext = nullptr;
	info.flags = 0;
	info.renderPass = renderPass->GetHandle();
	info.attachmentCount = rawAttachments.size();
	info.pAttachments = rawAttachments.data();
	info.width = width;
	info.height = height;
	info.layers = 1;

	VK_CHECK(vkCreateFramebuffer(mDevice.GetHandle(), &info, nullptr, &mHandle));
}
Framebuffer::~Framebuffer()
{
	vkDestroyFramebuffer(mDevice.GetHandle(), mHandle, nullptr);
}
const VkFramebuffer &Framebuffer::GetHandle() const
{
	return mHandle;
}