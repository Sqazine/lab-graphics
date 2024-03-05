#include "Attachment.h"

ColorAttachment::ColorAttachment()
    : mView(nullptr), mFormat(Format::R8G8B8A8_UNORM)
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
    mState.blendEnable = enable;
    mState.srcColorBlendFactor = BLEND_FACTOR_CAST(colorBlendDesc.srcFactor);
    mState.dstColorBlendFactor = BLEND_FACTOR_CAST(colorBlendDesc.dstFactor);
    mState.colorBlendOp = BLEND_OP_CAST(colorBlendDesc.op);
    mState.colorWriteMask = COLOR_COMPONENT_CAST(ColorComponent::ALL);
    mState.srcAlphaBlendFactor = BLEND_FACTOR_CAST(alphaBlendDesc.srcFactor);
    mState.dstAlphaBlendFactor = BLEND_FACTOR_CAST(alphaBlendDesc.dstFactor);
    mState.alphaBlendOp = BLEND_OP_CAST(alphaBlendDesc.op);
    return *this;
}

ColorAttachment &ColorAttachment::SetView(ImageView2D *view)
{
    mView = view;
    return *this;
}

const VkPipelineColorBlendAttachmentState &ColorAttachment::GetVkBlendState() const
{
    return mState;
}

const Format &ColorAttachment::GetFormat() const
{
    return mFormat;
}