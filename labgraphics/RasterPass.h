#pragma once
#include <memory>
#include <vector>
#include "Graphics/VK/CommandBuffer.h"
#include "Graphics/VK/SyncObject.h"
#include "Graphics/VK/Pipeline.h"

class RasterPass
{
public:
    RasterPass(size_t inFlightFrameCount);
    ~RasterPass();
    void Render();

    void RecordAllCommands(std::function<void(RasterCommandBuffer *,size_t)> fn);
    void RecordCurrentCommand(std::function<void(RasterCommandBuffer *,size_t)> fn);
private:
    std::vector<std::unique_ptr<RasterCommandBuffer>> mRasterCommandBuffers;
    std::vector<std::unique_ptr<Semaphore>> mImageAvailableSemaphores;
    std::vector<std::unique_ptr<Semaphore>> mRenderFinishedSemaphores;
    std::vector<std::unique_ptr<Fence>> mInFlightFences;
    size_t mInFlightFrameCount;
    size_t mCurFrame = 0;
};