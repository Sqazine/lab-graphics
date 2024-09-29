#include "App.h"
#include "Graphics/VK/CommandPool.h"
#include "Graphics/VK/CommandBuffer.h"

template <typename CmdBuffer>
inline typename std::enable_if_t<std::is_same_v<RasterCommandBuffer, CmdBuffer>,
                                 std::vector<std::unique_ptr<RasterCommandBuffer>>>
CreateCommandBuffers(size_t count)
{
    return std::move(App::Instance().GetGraphicsContext()->GetDevice()->GetRasterCommandPool()->CreatePrimaryCommandBuffers(count));
}

template <typename CmdBuffer>
inline typename std::enable_if_t<std::is_same_v<RayTraceCommandBuffer, CmdBuffer>,
                                 std::vector<std::unique_ptr<RayTraceCommandBuffer>>>
CreateCommandBuffers(size_t count)
{
    return std::move(App::Instance().GetGraphicsContext()->GetDevice()->GetRayTraceCommandPool()->CreatePrimaryCommandBuffers(count));
}

template <typename CmdBuffer>
Pass<CmdBuffer>::Pass(size_t inFlightFrameCount)
    : mInFlightFrameCount(inFlightFrameCount)
{
    mCommandBuffers = CreateCommandBuffers<CmdBuffer>(mInFlightFrameCount);
    mImageAvailableSemaphores = App::Instance().GetGraphicsContext()->GetDevice()->CreateSemaphores(mInFlightFrameCount);
    mRenderFinishedSemaphores = App::Instance().GetGraphicsContext()->GetDevice()->CreateSemaphores(mInFlightFrameCount);
    mInFlightFences = App::Instance().GetGraphicsContext()->GetDevice()->CreateFences(mInFlightFrameCount, FenceStatus::SIGNALED);
}

template <typename CmdBuffer>
Pass<CmdBuffer>::~Pass()
{
}

template <typename CmdBuffer>
void Pass<CmdBuffer>::Render()
{
    mInFlightFences[mCurFrame]->Wait();

    App::Instance().GetGraphicsContext()->GetSwapChain()->AcquireNextImage(mImageAvailableSemaphores[mCurFrame].get());

    mInFlightFences[mCurFrame]->Reset();

    GetCurrentCommandBuffer()->Submit({PipelineStage::COLOR_ATTACHMENT_OUTPUT}, {mImageAvailableSemaphores[mCurFrame].get()}, {mRenderFinishedSemaphores[mCurFrame].get()}, mInFlightFences[mCurFrame].get());

    App::Instance().GetGraphicsContext()->GetSwapChain()->Present({mRenderFinishedSemaphores[mCurFrame].get()});
    // App::Instance().GetGraphicsContext()->GetDevice()->GetPresentQueue()->WaitIdle();

    mCurFrame = (mCurFrame + 1) % mInFlightFrameCount;
}

template <typename CmdBuffer>
void Pass<CmdBuffer>::RecordAllCommands(std::function<void(CmdBuffer *, size_t)> fn)
{
    for (size_t i = 0; i < mCommandBuffers.size(); ++i)
        mCommandBuffers[i]->Record([&]()
                                   { fn(mCommandBuffers[i].get(), i); });
}

template <typename CmdBuffer>
void Pass<CmdBuffer>::RecordCurrentCommand(std::function<void(CmdBuffer *, size_t)> fn)
{
    mInFlightFences[mCurFrame]->Wait();
    mCommandBuffers[mCurFrame]->Record([&]()
                                       { fn(mCommandBuffers[mCurFrame].get(), mCurFrame); });
}

template <typename CmdBuffer>
CmdBuffer *Pass<CmdBuffer>::GetCurrentCommandBuffer() const
{
    return mCommandBuffers[mCurFrame].get();
}