#include "GraphicsContext.h"
#include "Base/App.h"
GraphicsContext::GraphicsContext()
{

    mInstance = std::make_unique<Instance>(App::Instance().GetWindow(), gValidationLayers,gInstanceExtensions);

    mDevice = std::make_unique<Device>(*mInstance,DeviceFeature::RAY_TRACE);

    mSwapChain = std::make_unique<SwapChain>(*mDevice);
   
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