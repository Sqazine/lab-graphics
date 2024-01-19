#include "RasterPass.h"
#include "App.h"
#include "Graphics/VK/CommandPool.h"
#include "Graphics/VK/CommandBuffer.h"

RasterPass::RasterPass()
{
    mRasterCommandBuffers = App::Instance().GetGraphicsContext()->GetDevice()->GetRasterCommandPool()->CreatePrimaryCommandBuffers(App::Instance().GetGraphicsContext()->GetSwapChain()->GetDefaultFrameBuffers().size());

    mImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    mRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    mInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    mImagesInFlight.resize(App::Instance().GetGraphicsContext()->GetSwapChain()->GetImages().size(), nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
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
    uint32_t swapChainImageIdx = App::Instance().GetGraphicsContext()->GetSwapChain()->AcquireNextImage(mImageAvailableSemaphores[mCurFrame].get());

    if (mImagesInFlight[swapChainImageIdx] != nullptr)
        mImagesInFlight[swapChainImageIdx]->Wait(VK_TRUE, UINT64_MAX);
    mImagesInFlight[swapChainImageIdx] = mInFlightFences[mCurFrame].get();

    mInFlightFences[mCurFrame]->Reset();

    mRasterCommandBuffers[swapChainImageIdx]->Submit({PipelineStage::COLOR_ATTACHMENT_OUTPUT}, {mImageAvailableSemaphores[mCurFrame].get()}, {mRenderFinishedSemaphores[mCurFrame].get()}, mInFlightFences[mCurFrame].get());
    mRasterCommandBuffers[swapChainImageIdx]->Present(swapChainImageIdx, {mRenderFinishedSemaphores[mCurFrame].get()});
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