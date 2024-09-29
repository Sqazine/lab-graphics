#pragma once
#include <vulkan/vulkan.h>
#include <map>
#include "RayTraceSBT.h"
#include "Shader.h"
#include "ShaderGroup.h"
#include "PipelineLayout.h"
#include "Math/Vector2.h"
#include "Format.h"
#include "RenderPass.h"
#include "Enum.h"
#include "Attachment.h"

class Pipeline
{
public:
	Pipeline(const class Device &device);
	virtual ~Pipeline();

	const VkPipeline &GetHandle();
	PipelineLayout *GetLayout() const;

protected:
	virtual void Build() = 0;

	const class Device &mDevice;

	PipelineLayout *mLayout;
	VkPipeline mHandle;
};

class RasterPipeline : public Pipeline
{
public:
	RasterPipeline(const class Device &device);
	~RasterPipeline();

	RasterPipeline &SetVertexShader(Shader *shader);
	RasterPipeline &SetTessellationControlShader(Shader *shader);
	RasterPipeline &SetTessellationEvaluationShader(Shader *shader);
	RasterPipeline &SetGeometryShader(Shader *shader);
	RasterPipeline &SetFragmentShader(Shader *shader);

	RasterPipeline &AddVertexInputBinding(uint32_t binding, uint32_t stride);
	RasterPipeline &AddVertexInputAttribute(uint32_t binding, uint32_t location, uint32_t offset, Format format);

	RasterPipeline &SetPrimitiveTopology(PrimitiveTopology topology);
	RasterPipeline &SetPrimitiveRestartEnable(bool isOpen);

	RasterPipeline &AddViewport(const Vector2f &startPos, const Vector2u32 &extent);
	RasterPipeline &AddScissor(const Vector2i32 &offset, const Vector2u32 &extent);

	RasterPipeline &SetCullMode(CullMode cullMode);
	RasterPipeline &SetPolygonMode(PolygonMode mode);
	RasterPipeline &SetFrontFace(FrontFace frontFace);
	RasterPipeline &SetLineWidth(float lw);

	RasterPipeline &SetRasterDiscardEnable(bool v);

	RasterPipeline &SetDepthBiasEnable(bool enable);
	RasterPipeline &SetDepthClampEnable(bool enable);

	RasterPipeline &SetDepthBiasClamp(float v);
	RasterPipeline &SetDepthBiasConstantFactor(float v);
	RasterPipeline &SetDepthBiasSlopeFactor(float v);

	const VkViewport &GetViewport(uint32_t i);
	const VkRect2D &GetScissor(uint32_t i);

	RasterPipeline &SetSampleCount(SampleCount msaa);
	RasterPipeline &SetAlphaToCoverageEnable(bool enable);
	RasterPipeline &SetAlphaToOneEnableEnable(bool enable);
	RasterPipeline &SetMinSampleShading(float v);

	RasterPipeline &SetPipelineLayout(PipelineLayout *layout);

	RasterPipeline &SetRenderPass(RenderPass *renderPass);

	RasterPipeline& SetColorAttachment(size_t slot,const ColorAttachment& attachment);

	VkPipelineDepthStencilStateCreateInfo pDepthStencilState{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};

private:
	void Build() override;

	std::vector<VkVertexInputBindingDescription> mInputBindingCache{};
	std::vector<VkVertexInputAttributeDescription> mInputAttributeCache{};

	PrimitiveTopology mPrimitiveTopology{PrimitiveTopology::POINT_LIST};
	bool mIsPrimitiveRestartEnable{false};

	std::vector<VkViewport> mViewportCache{};
	std::vector<VkRect2D> mScissorCache{};

	CullMode mCullMode{CullMode::NONE};
	PolygonMode mPolygonMode{PolygonMode::FILL};
	FrontFace mFrontFace{FrontFace::CCW};
	float mLineWidth{1.0};
	bool mRasterDiscardEnable{false};
	bool mDepthBiasEnable{false};
	bool mDepthClampEnable{false};
	float mDepthBiasClamp{0.0};
	float mDepthBiasConstantFactor{0.0};
	float mDepthBiasSlopeFactor{0.0};

	SampleCount mSampleCount{SampleCount::X1};
	bool mIsAlphaToCoverageEnable{false};
	bool mIsAlphaToOneEnableEnable{false};
	float mMinSampleShading{0};

	RenderPass *mRenderPass;

	std::map<size_t,VkPipelineColorBlendAttachmentState> mColorBlendAttachmentStates;

	RasterShaderGroup mShaderGroup;
};

class ComputePipeline : public Pipeline
{
public:
	ComputePipeline(const class Device &device);
	~ComputePipeline();

	ComputePipeline &SetShader(Shader *shader);

	ComputePipeline &SetPipelineLayout(PipelineLayout *layout);
private:
	void Build() override;
	ComputeShaderGroup mShaderGroup;
};

class RayTracePipeline : public Pipeline
{
public:
	RayTracePipeline(const class Device &device);
	~RayTracePipeline();

	RayTracePipeline &SetRayGenShader(Shader *shader);
	RayTracePipeline &AddRayClosestHitShader(Shader *shader);
	RayTracePipeline &AddRayMissShader(Shader *shader);
	RayTracePipeline &AddRayAnyHitShader(Shader *shader);
	RayTracePipeline &AddRayIntersectionShader(Shader *shader);
	RayTracePipeline &AddRayCallableShader(Shader *shader);

	const RayTraceSBT &GetSBT() const;

	RayTracePipeline &SetPipelineLayout(PipelineLayout *layout);
private:
	void Build() override;
	RayTraceShaderGroup mShaderGroup;
	RayTraceSBT mSBT;
};