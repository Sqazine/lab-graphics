#include "GraphicsContext.h"
#include "App.h"
GraphicsContext::GraphicsContext()
{
    mInstance = std::make_unique<Instance>(App::Instance().GetWindow(), gValidationLayers, gInstanceExtensions);

    mDevice = std::make_unique<Device>(*mInstance, DeviceFeature::RAY_TRACE);

    mSwapChain = std::make_unique<SwapChain>(*mDevice);
}
GraphicsContext::~GraphicsContext()
{
    mDevice->WaitIdle();
    mSwapChain.reset(nullptr);
    mDevice.reset(nullptr);
    mInstance.reset(nullptr);
}

Instance *GraphicsContext::GetInstance() const
{
    return mInstance.get();
}
Device *GraphicsContext::GetDevice() const
{
    return mDevice.get();
}
SwapChain *GraphicsContext::GetSwapChain()
{
    return mSwapChain.get();
}