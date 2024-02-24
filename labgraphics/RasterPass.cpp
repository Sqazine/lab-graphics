#include "RasterPass.h"
#include "App.h"
#include "Graphics/VK/CommandPool.h"
#include "Graphics/VK/CommandBuffer.h"

RasterPass::RasterPass()
{

    mImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    mRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    mInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
    mRasterCommandBuffers = App::Instance().GetGraphicsContext()->GetDevice()->GetRasterCommandPool()->CreatePrimaryCommandBuffers(MAX_FRAMES_IN_FLIGHT);
        mImageAvailableSemaphores[i] = App::Instance().GetGraphicsContext()->GetDevice()->CreateSemaphore();
        mRenderFinishedSemaphores[i] = App::Instance().GetGraphicsContext()->GetDevice()->CreateSemaphore();
        mInFlightFences[i] = App::Instance().GetGraphicsContext()->GetDevice()->CreateFence(FenceStatus::SIGNALED);
    }
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

    mCurFrame = (mCurFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void RasterPass::RecordCommand(std::function<void(RasterCommandBuffer *, uint32_t)> fn)
{
    for (size_t i = 0; i < mRasterCommandBuffers.size(); ++i)
    {
        mRasterCommandBuffers[i]->Record([&]()
                                         { fn(mRasterCommandBuffers[i].get(), i); });
    }
}