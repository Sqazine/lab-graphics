#include "RasterPass.h"
#include "App.h"
#include "Graphics/VK/CommandPool.h"
#include "Graphics/VK/CommandBuffer.h"

RasterPass::RasterPass(size_t inFlightFrameCount)
    : mInFlightFrameCount(inFlightFrameCount)
{

    mImageAvailableSemaphores.resize(mInFlightFrameCount);
    mRenderFinishedSemaphores.resize(mInFlightFrameCount);
    mInFlightFences.resize(mInFlightFrameCount);

    mRasterCommandBuffers = App::Instance().GetGraphicsContext()->GetDevice()->GetRasterCommandPool()->CreatePrimaryCommandBuffers(mInFlightFrameCount);
    mImageAvailableSemaphores = App::Instance().GetGraphicsContext()->GetDevice()->CreateSemaphores(mInFlightFrameCount);
    mRenderFinishedSemaphores = App::Instance().GetGraphicsContext()->GetDevice()->CreateSemaphores(mInFlightFrameCount);
    mInFlightFences = App::Instance().GetGraphicsContext()->GetDevice()->CreateFences(mInFlightFrameCount, FenceStatus::SIGNALED);
}

RasterPass::~RasterPass()
{
}

void RasterPass::Render()
{
    App::Instance().GetGraphicsContext()->GetSwapChain()->AcquireNextImage(mImageAvailableSemaphores[mCurFrame].get());

    mInFlightFences[mCurFrame]->Wait();
    mInFlightFences[mCurFrame]->Reset();

    mRasterCommandBuffers[mCurFrame]->Submit({PipelineStage::COLOR_ATTACHMENT_OUTPUT}, {mImageAvailableSemaphores[mCurFrame].get()}, {mRenderFinishedSemaphores[mCurFrame].get()}, mInFlightFences[mCurFrame].get());

    App::Instance().GetGraphicsContext()->GetSwapChain()->Present({mRenderFinishedSemaphores[mCurFrame].get()});
    App::Instance().GetGraphicsContext()->GetDevice()->GetPresentQueue()->WaitIdle();

    mCurFrame = (mCurFrame + 1) % mInFlightFrameCount;
}

void RasterPass::RecordAllCommands(std::function<void(RasterCommandBuffer *, size_t)> fn)
{
    for (size_t i = 0; i < mRasterCommandBuffers.size(); ++i)
    {
        mRasterCommandBuffers[i]->Record([&]()
                                         { fn(mRasterCommandBuffers[i].get(), i); });
    }
}

void RasterPass::RecordCurrentCommand(std::function<void(RasterCommandBuffer *, size_t)> fn)
{
    mRasterCommandBuffers[mCurFrame]->Record([&]()
                                             { fn(mRasterCommandBuffers[mCurFrame].get(), mCurFrame); });
}