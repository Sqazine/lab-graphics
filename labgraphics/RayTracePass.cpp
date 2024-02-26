#include "RayTracePass.h"
#include "App.h"
#include "Graphics/VK/CommandPool.h"
#include "Graphics/VK/CommandBuffer.h"

RayTracePass::RayTracePass(size_t inFlightFrameCount)
    : mInFlightFrameCount(inFlightFrameCount)
{

    mImageAvailableSemaphores.resize(mInFlightFrameCount);
    mRenderFinishedSemaphores.resize(mInFlightFrameCount);
    mInFlightFences.resize(mInFlightFrameCount);

    mRayTraceCommandBuffers = App::Instance().GetGraphicsContext()->GetDevice()->GetRayTraceCommandPool()->CreatePrimaryCommandBuffers(mInFlightFrameCount);
    mImageAvailableSemaphores = App::Instance().GetGraphicsContext()->GetDevice()->CreateSemaphores(mInFlightFrameCount);
    mRenderFinishedSemaphores = App::Instance().GetGraphicsContext()->GetDevice()->CreateSemaphores(mInFlightFrameCount);
    mInFlightFences = App::Instance().GetGraphicsContext()->GetDevice()->CreateFences(mInFlightFrameCount, FenceStatus::SIGNALED);
}

RayTracePass::~RayTracePass()
{
}

void RayTracePass::Render()
{
    App::Instance().GetGraphicsContext()->GetSwapChain()->AcquireNextImage(mImageAvailableSemaphores[mCurFrame].get());

    mInFlightFences[mCurFrame]->Wait();
    mInFlightFences[mCurFrame]->Reset();

    mRayTraceCommandBuffers[mCurFrame]->Submit({PipelineStage::COLOR_ATTACHMENT_OUTPUT}, {mImageAvailableSemaphores[mCurFrame].get()}, {mRenderFinishedSemaphores[mCurFrame].get()}, mInFlightFences[mCurFrame].get());

    App::Instance().GetGraphicsContext()->GetSwapChain()->Present({mRenderFinishedSemaphores[mCurFrame].get()});
    App::Instance().GetGraphicsContext()->GetDevice()->GetPresentQueue()->WaitIdle();

    mCurFrame = (mCurFrame + 1) % mInFlightFrameCount;
}

void RayTracePass::RecordAllCommands(std::function<void(RayTraceCommandBuffer *, size_t)> fn)
{
    for (size_t i = 0; i < mRayTraceCommandBuffers.size(); ++i)
    {
        mRayTraceCommandBuffers[i]->Record([&]()
                                           { fn(mRayTraceCommandBuffers[i].get(), i); });
    }
}

void RayTracePass::RecordCurrentCommand(std::function<void(RayTraceCommandBuffer *, size_t)> fn)
{
    mRayTraceCommandBuffers[mCurFrame]->Record([&]()
                                               { fn(mRayTraceCommandBuffers[mCurFrame].get(), mCurFrame); });
}