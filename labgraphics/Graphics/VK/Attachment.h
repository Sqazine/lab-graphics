#pragma once
#include "Format.h"
#include "Enum.h"
#include "ImageView.h"

struct BlendDesc
{
    BlendOp op = BlendOp::ADD;
    BlendFactor srcFactor = BlendFactor::ONE;
    BlendFactor dstFactor = BlendFactor::ZERO;
};

class Attachment
{
public:
    Attachment();
    ~Attachment();

    void SetFormat(Format fmt);
    void SetBlendDesc(bool enable, BlendDesc colorBlendDesc, BlendDesc alphaBlendDesc);

    void SetView(ImageView2D *view);

    const VkPipelineColorBlendAttachmentState &GetRaw() const;

    const Format &GetFormat() const;

private:
    Format mFormat;
    VkPipelineColorBlendAttachmentState mBlendDesc;
    ImageView2D *mView;
};