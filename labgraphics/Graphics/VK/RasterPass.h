#pragma once
#include <memory>
#include "RenderPass.h"
#include "Framebuffer.h"
#include "Pipeline.h"
#include "AttachmentDesc.h"
class RasterPass
{
public:
    RasterPass();
    ~RasterPass();

    AttachmentDesc& AddAttachmentDesc(); 

private:
    std::unique_ptr<RasterPipeline> mRasterPipeline;

    std::vector<std::unique_ptr<Framebuffer>> mFrameBuffers;
};