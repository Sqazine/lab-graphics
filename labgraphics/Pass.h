#pragma once
#include <memory>
#include <vector>
#include <cstdint>
#include "Graphics/VK/CommandBuffer.h"
#include "Graphics/VK/SyncObject.h"
#include "Graphics/VK/Pipeline.h"

template <typename CmdBuffer>
class Pass
{
public:
    Pass(size_t inFlightFrameCount);
    ~Pass();

    void Render();

    void RecordAllCommands(std::function<void(CmdBuffer *, size_t)> fn);
    void RecordCurrentCommand(std::function<void(CmdBuffer *, size_t)> fn);

    CmdBuffer* GetCurrentCommandBuffer() const;
private:
    std::vector<std::unique_ptr<CmdBuffer>> mCommandBuffers;
    std::vector<std::unique_ptr<Semaphore>> mImageAvailableSemaphores;
    std::vector<std::unique_ptr<Semaphore>> mRenderFinishedSemaphores;
    std::vector<std::unique_ptr<Fence>> mInFlightFences;
    size_t mInFlightFrameCount;
    size_t mCurFrame = 0;
};

#include "Pass.inl"

using RasterPass = Pass<RasterCommandBuffer>;
using RayTracePass = Pass<RayTraceCommandBuffer>;