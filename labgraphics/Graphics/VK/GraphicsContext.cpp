#include "GraphicsContext.h"
#include "Base/App.h"
GraphicsContext::GraphicsContext()
{

    mInstance = std::make_unique<Instance>(App::Instance().GetWindow(), gValidationLayers,gInstanceExtensions);

    mDevice = std::make_unique<Device>(*mInstance,DeviceFeature::RAY_TRACE);

    mSwapChain = std::make_unique<SwapChain>(*mDevice);
    mDefaultRenderPass = std::make_unique<RenderPass>(*mDevice, mSwapChain->GetFormat());

    for (size_t i = 0; i < mSwapChain->GetImageViews().size(); ++i)
    {
        std::vector<ImageView2D *> views = {mSwapChain->GetImageViews()[i].get()};
        mSwapChainFrameBuffers.emplace_back(std::make_unique<Framebuffer>(
            *mDevice,
            mDefaultRenderPass.get(),
            views,
            mSwapChain->GetExtent().x,
            mSwapChain->GetExtent().y));
    }
}
GraphicsContext::~GraphicsContext()
{
    mDevice->WaitIdle();
}

Instance *GraphicsContext::GetInstance() const
{
    return mInstance.get();
}
Device *GraphicsContext::GetDevice() const
{
    return mDevice.get();
}
SwapChain *GraphicsContext::GetSwapChain() const
{
    return mSwapChain.get();
}

RenderPass *GraphicsContext::GetDefaultRenderPass() const
{
    return mDefaultRenderPass.get();
}

const std::vector<std::unique_ptr<Framebuffer>> &GraphicsContext::GetDefaultFrameBuffers() const
{
    return mSwapChainFrameBuffers;
}