#pragma once
#include "Format.h"
#include "Enum.h"
#include "ImageView.h"

struct BlendDesc
{
    BlendOp op = BlendOp::ADD;
    BlendFactor srcFactor = BlendFactor::ZERO;
    BlendFactor dstFactor = BlendFactor::ONE;
};

class ColorAttachment
{
public:
    ColorAttachment();
    ~ColorAttachment();

    ColorAttachment &SetFormat(Format fmt);
    ColorAttachment &SetBlendDesc(bool enable, BlendDesc colorBlendDesc = {}, BlendDesc alphaBlendDesc = {});
    ColorAttachment &SetView(ImageView2D *view);

    const VkPipelineColorBlendAttachmentState& GetVkBlendState() const;

    const Format &GetFormat() const;
private:
    Format mFormat;
    ImageView2D *mView;

    VkPipelineColorBlendAttachmentState mState;
};