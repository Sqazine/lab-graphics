#pragma once

#include "VK/Utils.h"
#include "VK/Instance.h"
#include "VK/Device.h"
#include "PostProcessPass.h"
#include "Math/Matrix4.h"
#include "RaymanScene.h"
#include "VK/Image.h"
#include "VK/ImageView.h"
#include "VK/AS.h"
#include "ShaderCompiler.h"
#include <vulkan/vulkan.h>

enum class RenderState
{
	FINAL,
	AO,
	ALBEDO,
	NORMAL
};
struct Uniform
{
	alignas(64) Matrix4f view = Matrix4f::IDENTITY;
	alignas(64) Matrix4f projection = Matrix4f::IDENTITY;
	alignas(4) Vector3f cameraPos = Vector3f::ZERO;
	alignas(4) uint32_t lights{};
	alignas(4) uint32_t frame{};
	alignas(4) float hdrResolution{};
	alignas(4) float hdrMultiplier{};
};

class RtxRayTracePass
{
public:
	RtxRayTracePass(const Instance &instance, Device &device);
	~RtxRayTracePass();

	const Image2D *GetOutputImage() const;

	Device &GetDevice() const;

	void SetScene(class RtxRayTraceScene *scene);
	void SetPostProcessType(PostProcessType t);
	void SetRenderState(RenderState state);

	void ResetAccumulation();
	void SaveOutputImageToDisk();

	void CompileShaders() const;

	void Update();
	void Render();

	void CreateRayTracerPipeline();
	void CreateComputePipeline();
	void CreateBLAS();
	void CreateTLAS();

	void UpdateUniformBuffer(size_t frameIdx);

private:
	void BuildPipeline();
	void Copy(RayTraceCommandBuffer *commandBuffer, Image2D *src, VkImage dst) const;

	Device &mDevice;
	class RtxRayTraceScene *mScene;
	
	std::unique_ptr<RayTracePass> mRayTracePass;

	RenderState mState;
	RenderState mPrevState;

	std::unique_ptr<TLAS> mTLAS;
	std::vector<std::unique_ptr<BLAS>> mBLASs;

	std::vector<std::unique_ptr<UniformBuffer<Uniform>>> mUniformBuffers;

	std::unique_ptr<GpuImage2D> mAccumulationImage;
	std::unique_ptr<GpuImage2D> mOutputImage;
	std::unique_ptr<GpuImage2D> mNormalsImage;
	std::unique_ptr<GpuImage2D> mPositionsImage;

	std::unique_ptr<GpuBuffer> mBLASBuffer;
	std::unique_ptr<GpuBuffer> mScratchBLASBuffer;

	std::unique_ptr<RayTracePipeline> mPipeline{};
	std::unique_ptr<PipelineLayout> mPipelineLayout{};

	std::unique_ptr<DescriptorTable> mDescriptorTable;
	std::vector<DescriptorSet *> mDescriptorSets;

	std::unique_ptr<ShaderCompiler> mCompiler;

	std::unique_ptr<PostProcessPass> mPostProcessPass;
	PostProcessType mPostProcessType = PostProcessType::NONE;

	uint32_t mFrame = 0;

};
