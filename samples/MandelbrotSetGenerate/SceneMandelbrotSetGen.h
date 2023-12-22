#pragma once
#include "labgraphics.h"

constexpr uint32_t WIDTH = 3200;
constexpr uint32_t HEIGHT = 2400;
constexpr uint32_t WORKGROUP_SIZE = 32;

class SceneMandelbrotSetGen : public Scene
{
public:
    SceneMandelbrotSetGen()=default;
    ~SceneMandelbrotSetGen()=default;

    void Init() override;
    void Render() override;
private:
	void SaveImage(std::string_view fileName);

	std::unique_ptr<DescriptorTable> mComputeDescriptorTable;

	DescriptorSet* mComputeDescriptorSet;
	std::unique_ptr<PipelineLayout> mComputePipelineLayout;

	std::unique_ptr<ComputePipeline> mComputePipeline;

	std::unique_ptr<ComputeCommandBuffer> mComputeCommandBuffer;

	std::unique_ptr<Buffer> mImgBuffer;
};