#include "Attachment.h"

ColorAttachment::ColorAttachment()
    : mView(nullptr), mFormat(Format::R8G8B8A8_UNORM), mBlendEnable(false)
{
}

ColorAttachment::~ColorAttachment()
{
}

ColorAttachment &ColorAttachment::SetFormat(Format fmt)
{
    mFormat = fmt;
    return *this;
}

ColorAttachment &ColorAttachment::SetBlendDesc(bool enable, BlendDesc colorBlendDesc, BlendDesc alphaBlendDesc)
{
    mBlendEnable = enable;
    mColorBlendDesc = colorBlendDesc;
    mAlphaBlendDesc = alphaBlendDesc;
    return *this;
}

ColorAttachment &ColorAttachment::SetView(ImageView2D *view)
{
    mView = view;
    return *this;
}

VkPipelineColorBlendAttachmentState ColorAttachment::GetVkBlendState() const
{
    VkPipelineColorBlendAttachmentState result = {};
    result.blendEnable = mBlendEnable;
    result.srcColorBlendFactor = BLEND_FACTOR_CAST(mColorBlendDesc.srcFactor);
    result.dstColorBlendFactor = BLEND_FACTOR_CAST(mColorBlendDesc.dstFactor);
    result.colorBlendOp = BLEND_OP_CAST(mColorBlendDesc.op);
    result.colorWriteMask = COLOR_COMPONENT_CAST(ColorComponent::ALL);
    result.srcAlphaBlendFactor = BLEND_FACTOR_CAST(mAlphaBlendDesc.srcFactor);
    result.dstAlphaBlendFactor = BLEND_FACTOR_CAST(mAlphaBlendDesc.dstFactor);
    result.alphaBlendOp = BLEND_OP_CAST(mAlphaBlendDesc.op);

    return result;
}

const Format &ColorAttachment::GetFormat() const
{
    return mFormat;
}