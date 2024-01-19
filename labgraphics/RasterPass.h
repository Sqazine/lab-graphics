#pragma once
#include <memory>
#include <vector>
#include "Graphics/VK/CommandBuffer.h"
#include "Graphics/VK/SyncObject.h"
#include "Graphics/VK/Pipeline.h"
#define MAX_FRAMES_IN_FLIGHT 2

class RasterPass
{
public:
    RasterPass();
    ~RasterPass();
    void Render();

    void RecordCommand(std::function<void(RasterCommandBuffer *,uint32_t)> fn);

private:
    std::vector<std::unique_ptr<RasterCommandBuffer>> mRasterCommandBuffers;
    std::vector<std::unique_ptr<Semaphore>> mImageAvailableSemaphores;
    std::vector<std::unique_ptr<Semaphore>> mRenderFinishedSemaphores;
    std::vector<std::unique_ptr<Fence>> mInFlightFences;
    std::vector<Fence *> mImagesInFlight;
    size_t mCurFrame = 0;
};