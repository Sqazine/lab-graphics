#pragma once
#include "Format.h"
#include "Enum.h"
#include "ImageView.h"

class AttachmentDesc
{
public:
    AttachmentDesc();
    ~AttachmentDesc();

    void SetFormat(Format fmt);
    void SetColorBlendDesc(bool enable, BlendFactor src, BlendFactor dst);
    void SetAlphaBlendDesc(bool enable, BlendFactor src, BlendFactor dst);

    void SetView(ImageView2D *view);

    const VkPipelineColorBlendAttachmentState &GetRaw() const;

    const Format& GetFormat() const;
private:
    Format mFormat;
    VkPipelineColorBlendAttachmentState mBlendDesc;
    ImageView2D *mView;
};