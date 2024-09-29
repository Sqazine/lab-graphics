#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include <deque>
#include <functional>
#include <array>
#include <memory>
#include "labgraphics.h"


#define PARTICLE_NUM 20000
#define PARTICLE_RADIUS 0.005f
#define WORK_GROUP_SIZE 128
#define WORK_GROUP_NUM ((PARTICLE_NUM + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE)

class SceneSph : public Scene
{
public:
	SceneSph();
	~SceneSph();

private:
	void Init() override;
	void Update() override;
	void Simulate();
	void Render() override;

	void InitParticleData(std::array<Vector2f, PARTICLE_NUM> initParticlePosition);

	std::unique_ptr<PipelineLayout> mRasterPipelineLayout;
	std::unique_ptr<RasterPipeline> mRasterPipeline;

	std::unique_ptr<RasterPass> mSphRasterPass;

	std::unique_ptr<DescriptorTable> mComputeDescriptorTable;
	DescriptorSet* mComputeDescriptorSet;
	std::unique_ptr<PipelineLayout> mComputePipelineLayout;
	std::array<std::unique_ptr<ComputePipeline>, 3> mComputePipelines;

	std::unique_ptr<ComputeCommandBuffer> mComputeCommandBuffer;

	std::unique_ptr<GpuBuffer> mPositionBuffer;
	std::unique_ptr<GpuBuffer> mVelocityBuffer;
	std::unique_ptr<GpuBuffer> mForceBuffer;
	std::unique_ptr<GpuBuffer> mDensityBuffer;
	std::unique_ptr<GpuBuffer> mPressureBuffer;

	const uint64_t mPosSsboSize = sizeof(Vector2f) * PARTICLE_NUM;
	const uint64_t mVelocitySsboSize = sizeof(Vector2f) * PARTICLE_NUM;
	const uint64_t mForceSsboSize = sizeof(Vector2f) * PARTICLE_NUM;
	const uint64_t mDensitySsboSize = sizeof(float) * PARTICLE_NUM;
	const uint64_t mPressureSsboSize = sizeof(float) * PARTICLE_NUM;
};
