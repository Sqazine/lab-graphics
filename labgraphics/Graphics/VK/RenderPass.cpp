#include "RenderPass.h"
#include "Device.h"
RenderPass::RenderPass(const Device &device, const std::vector<Format> &colorformats, Format depthStencilFormat)
    : mDevice(device)
{
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> colorReferences;
    for (int32_t i = 0; i < colorformats.size(); ++i)
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.flags = 0;
        colorAttachment.format = colorformats[i].ToVkHandle();
        colorAttachment.samples = SAMPLE_COUNT(SampleCount::X1);
        colorAttachment.loadOp = ATTACHMENT_LOAD_CAST(AttachmentLoad::CLEAR);
        colorAttachment.storeOp = ATTACHMENT_STORE_CAST(AttachmentStore::STORE);
        colorAttachment.stencilLoadOp = ATTACHMENT_LOAD_CAST(AttachmentLoad::DONT_CARE);
        colorAttachment.stencilStoreOp = ATTACHMENT_STORE_CAST(AttachmentStore::STORE);
        colorAttachment.initialLayout = IMAGE_LAYOUT_CAST(ImageLayout::UNDEFINED);
        if (colorformats.size() == 1)
            colorAttachment.finalLayout = IMAGE_LAYOUT_CAST(ImageLayout::PRESENT_SRC_KHR);
        else
            colorAttachment.finalLayout = IMAGE_LAYOUT_CAST(ImageLayout::COLOR_ATTACHMENT_OPTIMAL);

        attachments.emplace_back(colorAttachment);

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = i;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        colorReferences.emplace_back(colorAttachmentRef);
    }

    VkSubpassDependency colorDependency{};
    colorDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    colorDependency.dstSubpass = 0;
    colorDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    colorDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    colorDependency.srcAccessMask = 0;
    colorDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::vector<VkSubpassDependency> dependencies = {
        colorDependency,
    };

    VkSubpassDescription subpass{};
    subpass.flags = 0;
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = colorReferences.size();
    subpass.pColorAttachments = colorReferences.data();
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    subpass.pPreserveAttachments = nullptr;
    subpass.preserveAttachmentCount = 0;
    subpass.pResolveAttachments = nullptr;

    if (depthStencilFormat != Format::UNDEFINED)
    {
        VkAttachmentDescription depthStencilAttachment{};
        depthStencilAttachment.flags = 0;
        depthStencilAttachment.format = depthStencilFormat.ToVkHandle();
        depthStencilAttachment.samples = SAMPLE_COUNT(SampleCount::X1);
        depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthStencilAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthStencilAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthStencilAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthStencilAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        attachments.emplace_back(depthStencilAttachment);

        VkAttachmentReference depthStencilAttachmentRef{};
        depthStencilAttachmentRef.attachment = attachments.size();
        depthStencilAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDependency depthDependency{};
        depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        depthDependency.dstSubpass = 0;
        depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        depthDependency.srcAccessMask = 0;
        depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        dependencies.emplace_back(depthDependency);

        subpass.pDepthStencilAttachment = &depthStencilAttachmentRef;
    }

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pNext = nullptr;
    renderPassInfo.flags = 0;
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = dependencies.size();
    renderPassInfo.pDependencies = dependencies.data();

    VK_CHECK(vkCreateRenderPass(device.GetHandle(), &renderPassInfo, nullptr, &mHandle))
}

RenderPass::RenderPass(const Device &device, const Format &colorformat, Format depthStencilFormat)
    : RenderPass(device, std::vector<Format>{colorformat}, depthStencilFormat)
{
}

RenderPass::~RenderPass()
{
    vkDestroyRenderPass(mDevice.GetHandle(), mHandle, nullptr);
}

const VkRenderPass &RenderPass::GetHandle() const
{
    return mHandle;
}
