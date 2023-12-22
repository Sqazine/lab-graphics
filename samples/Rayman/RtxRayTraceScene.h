#pragma once
#include "RaymanScene.h"
#include "RtxRayTracePass.h"
class RtxRayTraceScene
{
public:
    RtxRayTraceScene(RaymanScene *scene);
    ~RtxRayTraceScene();

    void Build();
    void CreateBuffers();
    void Fill(std::unique_ptr<class GpuBuffer> &buffer, void *data, size_t size, BufferUsage usage) const;

    const Buffer *GetVertexBuffer() const
    {
        return mVertexBuffer.get();
    }

    const Buffer *GetIndexBuffer() const
    {
        return mIndexBuffer.get();
    }

    const Buffer *GetMaterialBuffer() const
    {
        return mMaterialBuffer.get();
    }

    const Buffer *GetOffsetBuffer() const
    {
        return mOffsetBuffer.get();
    }

    const Buffer *GetLightsBuffer() const
    {
        return mLightsBuffer.get();
    }

    const std::vector<std::unique_ptr<class Texture>> &GetTextures() const
    {
        return mTextureImages;
    }

    const std::vector<std::unique_ptr<class Texture>> &GetHDRTextures() const
    {
        return mHdrImages;
    }

    bool UseHDR() const
    {
        return !mHdrImages.empty();
    }

    uint32_t GetTextureSize() const
    {
        return (uint32_t)mTextureImages.size();
    }

    RaymanScene *Get()
    {
        return mScene;
    }

    void ResetAccumulation();

    void SetRenderState(RenderState state);
    void SetPostProcessType(PostProcessType type);

    void SaveOutputImageToDisk();

    void Update();
    void Render();

private:
    RaymanScene *mScene;

    std::unique_ptr<class RtxRayTracePass> mRtxRayTracePass;

    std::vector<std::unique_ptr<Texture>> mTextureImages;
    std::vector<std::unique_ptr<Texture>> mHdrImages;

    std::unique_ptr<class GpuBuffer> mVertexBuffer;
    std::unique_ptr<class GpuBuffer> mIndexBuffer;
    std::unique_ptr<class GpuBuffer> mMaterialBuffer;
    std::unique_ptr<class GpuBuffer> mOffsetBuffer;
    std::unique_ptr<class GpuBuffer> mLightsBuffer;
};
