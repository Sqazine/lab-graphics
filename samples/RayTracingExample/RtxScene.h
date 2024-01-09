#pragma once
#include <vulkan/vulkan.h>
#include "Camera.h"
#include <vector>
#include "Math/Vector2.h"
#include <memory>
#include "SBT.h"
#include "labgraphics.h"

struct VertexAttribute
{
    Vector4f normal;
    Vector4f uv;
};

struct UniformParams
{
    Vector4f camPos;
    Vector4f camDir;
    Vector4f camUp;
    Vector4f camSide;
    Vector4f camNearFarFov;
    uint32_t currentSamplesCount = 0;
};

struct AccelerationStructure
{
	VkUtils::Buffer buffer;
	VkAccelerationStructureKHR accelerationStructure;
	VkDeviceAddress address;
};

struct _Mesh
{
	uint32_t numVertices;
	uint32_t numFaces;

	VkUtils::Buffer positionBuffer;
	VkUtils::Buffer attributeBuffer;
	VkUtils::Buffer indexBuffer;
	VkUtils::Buffer faceBuffer;
	VkUtils::Buffer materialIDBuffer;

    AccelerationStructure blas;
};

struct _Material
{
	VkUtils::Image albedo;
};

struct AccelStructScene
{
	std::vector<_Mesh> meshes;
	std::vector<_Material> materials;

	AccelerationStructure tlas;

	std::vector<VkDescriptorBufferInfo> materialIDBufferInfos;
	std::vector<VkDescriptorBufferInfo> attributeBufferInfos;
	std::vector<VkDescriptorBufferInfo> faceBufferInfos;
	std::vector<VkDescriptorImageInfo> textureImageInfos;

	void BuildBLAS(Device* device);
	void BuildTLAS(Device* device);
};


class RtxScene : public Scene
{
public:
    RtxScene();
    ~RtxScene();

    void Init() override;
    void ProcessInput() override;
    void Update() override;
    void Render() override;
    void RenderUI() override;
    void CleanUp() override;
private:
    void FillCommandBuffers();

    void LoadSceneGeometry();
    void CreateScene();
    void CreateCamera();
    void UpdateCameraParams(struct UniformParams *params, const float dt);
    void CreateDescriptorSetLayouts();
    void CreateRayTracingPipelineAndSBT();
    void UpdateDescriptorSets();

    std::vector<std::unique_ptr<Fence>> mWaitForFrameFences;
    VkUtils::Image mOffscreenImage;
    std::vector<std::unique_ptr<RayTraceCommandBuffer>> mCommandBuffers;
    std::unique_ptr<Semaphore> mSemaphoreImageAcquired;
    std::unique_ptr<Semaphore> mSemaphoreRenderFinished;

    std::vector<VkDescriptorSetLayout> mDescriptorSetsLayouts;
    VkPipelineLayout mPipelineLayout;
    VkPipeline mPipeline;
    VkDescriptorPool mDescriptorPool;
    std::vector<VkDescriptorSet> mDescriptorSets;

    SBT mSBT;

    AccelStructScene mScene;
    VkUtils::Image mEnvTexture;
    VkDescriptorImageInfo mEnvTextureDescInfo;

    _Camera mCamera;
    VkUtils::Buffer mCameraBuffer;

    Vector2f mCursorPos;
};