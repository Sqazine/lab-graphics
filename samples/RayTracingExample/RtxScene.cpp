#include "RtxScene.h"
#include "VkUtils.h"
#include "labgraphics.h"
#include <iostream>
#include <cassert>
#include <string>
#include <tinyobjloader/tiny_obj_loader.h>
#include "App.h"

void AccelStructScene::BuildBLAS(Device *device)
{
	const size_t numMeshes = meshes.size();

	std::vector<VkAccelerationStructureGeometryKHR> geometries(numMeshes, VkAccelerationStructureGeometryKHR{});
	std::vector<VkAccelerationStructureBuildRangeInfoKHR> ranges(numMeshes, VkAccelerationStructureBuildRangeInfoKHR{});
	std::vector<VkAccelerationStructureBuildGeometryInfoKHR> buildInfos(numMeshes, VkAccelerationStructureBuildGeometryInfoKHR{});
	std::vector<VkAccelerationStructureBuildSizesInfoKHR> sizeInfos(numMeshes, VkAccelerationStructureBuildSizesInfoKHR{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR});

	for (size_t i = 0; i < numMeshes; ++i)
	{
		_Mesh &mesh = meshes[i];

		auto &geometry = geometries[i];
		auto &range = ranges[i];
		auto &buildInfo = buildInfos[i];

		range.primitiveCount = mesh.numFaces;

		geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		geometry.pNext = nullptr;
		geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		geometry.geometry.triangles.vertexData = VkUtils::GetBufferDeviceAddressConst(mesh.positionBuffer);
		geometry.geometry.triangles.vertexStride = sizeof(Vector3f);
		geometry.geometry.triangles.maxVertex = mesh.numVertices;
		geometry.geometry.triangles.indexData = VkUtils::GetBufferDeviceAddressConst(mesh.indexBuffer);
		geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;

		buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		buildInfo.pNext = nullptr;
		buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		buildInfo.geometryCount = 1;
		buildInfo.pGeometries = &geometry;

		device->vkGetAccelerationStructureBuildSizesKHR(device->GetHandle(),
														VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
														&buildInfo,
														&range.primitiveCount,
														&sizeInfos[i]);
	}

	VkDeviceSize maximumBlasSize = 0;
	for (const auto &sizeInfo : sizeInfos)
		maximumBlasSize = Math::Max(sizeInfo.buildScratchSize, maximumBlasSize);

	VkUtils::Buffer scratchBuffer;
	VK_CHECK(scratchBuffer.Create(maximumBlasSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

	auto cmd = App::Instance().GetGraphicsContext()->GetDevice()->GetRayTraceCommandPool()->CreatePrimaryCommandBuffer();
	cmd->ExecuteImmediately([&]()
							{

		VkMemoryBarrier memoryBarrier{};
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.pNext = nullptr;
		memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
		memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

		for (size_t i = 0; i < numMeshes; ++i)
		{
			_Mesh& mesh = meshes[i];
			mesh.blas.buffer.Create(sizeInfos[i].accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			VkAccelerationStructureCreateInfoKHR createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
			createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
			createInfo.size = sizeInfos[i].accelerationStructureSize;
			createInfo.buffer = mesh.blas.buffer.GetBuffer();

			VK_CHECK(device->vkCreateAccelerationStructureKHR(device->GetHandle(), &createInfo, nullptr, &mesh.blas.accelerationStructure));

			VkAccelerationStructureBuildGeometryInfoKHR& buildInfo = buildInfos[i];
			buildInfo.scratchData = VkUtils::GetBufferDeviceAddress(scratchBuffer);
			buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
			buildInfo.dstAccelerationStructure = mesh.blas.accelerationStructure;

			const VkAccelerationStructureBuildRangeInfoKHR* range[1] = { &ranges[i] };

			device->vkCmdBuildAccelerationStructuresKHR(cmd->GetHandle(), 1, &buildInfo, range);

			vkCmdPipelineBarrier(cmd->GetHandle(),
				VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
				VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
				0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
		} });

	for (size_t i = 0; i < numMeshes; ++i)
	{
		_Mesh &mesh = meshes[i];

		VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
		addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		addressInfo.accelerationStructure = mesh.blas.accelerationStructure;
		mesh.blas.address = device->vkGetAccelerationStructureDeviceAddressKHR(device->GetHandle(), &addressInfo);
	}
}
void AccelStructScene::BuildTLAS(Device *device)
{
	const VkTransformMatrixKHR transform =
		{
			1.0f,
			0.0f,
			0.0f,
			0.0f,
			0.0f,
			1.0f,
			0.0f,
			0.0f,
			0.0f,
			0.0f,
			1.0f,
			0.0f,
		};

	const size_t numMeshes = meshes.size();

	std::vector<VkAccelerationStructureInstanceKHR> instances(numMeshes, VkAccelerationStructureInstanceKHR{});
	for (size_t i = 0; i < numMeshes; ++i)
	{
		_Mesh &mesh = meshes[i];

		VkAccelerationStructureInstanceKHR &instance = instances[i];
		instance.transform = transform;
		instance.instanceCustomIndex = i;
		instance.mask = 0xff;
		instance.instanceShaderBindingTableRecordOffset = 0;
		instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		instance.accelerationStructureReference = mesh.blas.address;
	}

	VkUtils::Buffer instancesBuffer;
	VK_CHECK(instancesBuffer.Create(instances.size() * sizeof(VkAccelerationStructureInstanceKHR),
									VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
									VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

	if (!instancesBuffer.UploadData(instances.data(), instancesBuffer.GetSize()))
		assert(false && "Failed to upload instances buffer");

	VkAccelerationStructureGeometryInstancesDataKHR tlasInstancesInfo{};
	tlasInstancesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	tlasInstancesInfo.pNext = nullptr;
	tlasInstancesInfo.data = VkUtils::GetBufferDeviceAddressConst(instancesBuffer);

	VkAccelerationStructureGeometryKHR tlasGeoInfo{};
	tlasGeoInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	tlasGeoInfo.pNext = nullptr;
	tlasGeoInfo.flags = 0;
	tlasGeoInfo.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	tlasGeoInfo.geometry.instances = tlasInstancesInfo;

	VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	buildInfo.geometryCount = 1;
	buildInfo.pGeometries = &tlasGeoInfo;

	const uint32_t numInstances = instances.size();

	VkAccelerationStructureBuildSizesInfoKHR sizeInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
	device->vkGetAccelerationStructureBuildSizesKHR(device->GetHandle(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &numInstances, &sizeInfo);

	tlas.buffer.Create(sizeInfo.accelerationStructureSize,
					   VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
					   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkAccelerationStructureCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	createInfo.size = sizeInfo.accelerationStructureSize;
	createInfo.buffer = tlas.buffer.GetBuffer();

	VK_CHECK(device->vkCreateAccelerationStructureKHR(device->GetHandle(), &createInfo, nullptr, &tlas.accelerationStructure));

	VkUtils::Buffer scratchBuffer;
	VK_CHECK(scratchBuffer.Create(sizeInfo.buildScratchSize,
								  VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
								  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

	buildInfo.scratchData = VkUtils::GetBufferDeviceAddress(scratchBuffer);
	buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
	buildInfo.dstAccelerationStructure = tlas.accelerationStructure;

	auto cmd = App::Instance().GetGraphicsContext()->GetDevice()->GetRayTraceCommandPool()->CreatePrimaryCommandBuffer();
	cmd->ExecuteImmediately([&]()
							{
		VkAccelerationStructureBuildRangeInfoKHR range = {};
		range.primitiveCount = numInstances;

		const VkAccelerationStructureBuildRangeInfoKHR* ranges[1] = { &range };

		device->vkCmdBuildAccelerationStructuresKHR(cmd->GetHandle(), 1, &buildInfo, ranges); });

	VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
	addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	addressInfo.accelerationStructure = tlas.accelerationStructure;
	tlas.address = device->vkGetAccelerationStructureDeviceAddressKHR(device->GetHandle(), &addressInfo);
}

static const float moveSpeed = 20.0f;
static const float accelMult = 20.0f;
static const float rotateSpeed = 20.0f;

RtxScene::RtxScene()
{
#ifdef _DEBUG
	std::string cmd = "glslangValidator --target-env vulkan1.2 -V -S rgen  " + std::string(RAY_TRACING_EXAMPLE_DIR) + "shader.rgen -o shader.rgen.spv";
	std::system(cmd.c_str());

	cmd = "glslangValidator --target-env vulkan1.2 -V -S rchit  " + std::string(RAY_TRACING_EXAMPLE_DIR) + "shader.rchit -o shader.rchit.spv";
	std::system(cmd.c_str());

	cmd = "glslangValidator --target-env vulkan1.2 -V -S rmiss  " + std::string(RAY_TRACING_EXAMPLE_DIR) + "shader.rmiss -o shader.rmiss.spv";
	std::system(cmd.c_str());

	cmd = "glslangValidator --target-env vulkan1.2 -V -S rchit  " + std::string(RAY_TRACING_EXAMPLE_DIR) + "shadow.rchit -o shadow.rchit.spv";
	std::system(cmd.c_str());

	cmd = "glslangValidator --target-env vulkan1.2 -V -S rmiss  " + std::string(RAY_TRACING_EXAMPLE_DIR) + "shadow.rmiss -o shadow.rmiss.spv";
	std::system(cmd.c_str());
#endif
}
RtxScene::~RtxScene()
{
}

void RtxScene::Init()
{
	App::Instance().GetWindow()->Show();
	App::Instance().GetWindow()->SetTitle("ray-tracing-example");

	mWaitForFrameFences.resize(App::Instance().GetGraphicsContext()->GetSwapChain()->GetImages().size());
	for (auto &fence : mWaitForFrameFences)
		fence = std::make_unique<Fence>(*App::Instance().GetGraphicsContext()->GetDevice(), FenceStatus::SIGNALED);

	const VkExtent3D extent = {App::Instance().GetWindow()->GetExtent().x, App::Instance().GetWindow()->GetExtent().y, 1};

	VK_CHECK(mOffscreenImage.Create(VK_IMAGE_TYPE_2D,
									App::Instance().GetGraphicsContext()->GetSwapChain()->GetFormat().ToVkHandle(),
									extent,
									VK_IMAGE_TILING_OPTIMAL,
									VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
									VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

	VkImageSubresourceRange range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
	VK_CHECK(mOffscreenImage.CreateImageView(VK_IMAGE_VIEW_TYPE_2D, App::Instance().GetGraphicsContext()->GetSwapChain()->GetFormat().ToVkHandle(), range));

	mCommandBuffers = App::Instance().GetGraphicsContext()->GetDevice()->GetRayTraceCommandPool()->CreatePrimaryCommandBuffers(App::Instance().GetGraphicsContext()->GetSwapChain()->GetImages().size());
	mSemaphoreImageAcquired = std::make_unique<Semaphore>(*App::Instance().GetGraphicsContext()->GetDevice());
	mSemaphoreRenderFinished = std::make_unique<Semaphore>(*App::Instance().GetGraphicsContext()->GetDevice());

	LoadSceneGeometry();
	CreateScene();
	CreateCamera();
	CreateDescriptorSetLayouts();
	CreateRayTracingPipelineAndSBT();
	UpdateDescriptorSets();

	FillCommandBuffers();
}
void RtxScene::ProcessInput()
{
}

void RtxScene::Update()
{
	UniformParams *params = reinterpret_cast<UniformParams *>(mCameraBuffer.Map());
	params->currentSamplesCount++;
	UpdateCameraParams(params, App::Instance().GetTimer().GetDeltaTime());
	mCameraBuffer.Unmap();
}

void RtxScene::Render()
{
	App::Instance().GetGraphicsContext()->GetSwapChain()->AcquireNextImage(mSemaphoreImageAcquired.get());

	Fence *fence = mWaitForFrameFences[App::Instance().GetGraphicsContext()->GetSwapChain()->GetNextImageIdx()].get();

	fence->Wait(VK_TRUE, UINT64_MAX);
	fence->Reset();

	const PipelineStage waitStageMask = PipelineStage::COLOR_ATTACHMENT_OUTPUT;

	mCommandBuffers[App::Instance().GetGraphicsContext()->GetSwapChain()->GetNextImageIdx()]->Submit({waitStageMask}, {mSemaphoreImageAcquired.get()}, {mSemaphoreRenderFinished.get()}, fence);

	App::Instance().GetGraphicsContext()->GetSwapChain()->Present({mSemaphoreRenderFinished.get()});
}

void RtxScene::RenderUI()
{
}

void RtxScene::CleanUp()
{
	for (_Mesh &mesh : mScene.meshes)
		App::Instance().GetGraphicsContext()->GetDevice()->vkDestroyAccelerationStructureKHR(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), mesh.blas.accelerationStructure, nullptr);

	mScene.meshes.clear();
	mScene.materials.clear();

	if (mScene.tlas.accelerationStructure)
	{
		App::Instance().GetGraphicsContext()->GetDevice()->vkDestroyAccelerationStructureKHR(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), mScene.tlas.accelerationStructure, nullptr);
		mScene.tlas.accelerationStructure = VK_NULL_HANDLE;
	}

	if (mDescriptorPool)
	{
		vkDestroyDescriptorPool(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), mDescriptorPool, nullptr);
		mDescriptorPool = VK_NULL_HANDLE;
	}

	mSBT.Destroy();

	if (mPipeline)
	{
		vkDestroyPipeline(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), mPipeline, nullptr);
		mPipeline = VK_NULL_HANDLE;
	}

	if (mPipelineLayout)
	{
		vkDestroyPipelineLayout(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), mPipelineLayout, nullptr);
		mPipelineLayout = VK_NULL_HANDLE;
	}

	for (VkDescriptorSetLayout &dsl : mDescriptorSetsLayouts)
		vkDestroyDescriptorSetLayout(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), dsl, nullptr);
	mDescriptorSetsLayouts.clear();

	mWaitForFrameFences.clear();

	mOffscreenImage.Destroy();
	vkDeviceWaitIdle(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle());
}

void RtxScene::FillCommandBuffers()
{
	VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

	for (size_t i = 0; i < mCommandBuffers.size(); ++i)
	{
		mCommandBuffers[i]->Record([&]()
								   {
			mCommandBuffers[i]->ImageBarrier(mOffscreenImage.GetImage(), Access::NONE, Access::SHADER_WRITE, ImageLayout::UNDEFINED, ImageLayout::GENERAL, subresourceRange);

			vkCmdBindPipeline(mCommandBuffers[i]->GetHandle(), VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, mPipeline);
			vkCmdBindDescriptorSets(mCommandBuffers[i]->GetHandle(), VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, mPipelineLayout, 0, mDescriptorSets.size(), mDescriptorSets.data(), 0, 0);

			VkStridedDeviceAddressRegionKHR rayGenRegion{};
			rayGenRegion.deviceAddress = mSBT.GetAddress() + mSBT.GetRayGenOffset();
			rayGenRegion.stride = mSBT.GetGroupsStride();
			rayGenRegion.size = mSBT.GetRayGenSize();

			VkStridedDeviceAddressRegionKHR missRegion{};
			missRegion.deviceAddress = mSBT.GetAddress() + mSBT.GetMissGroupsOffset();
			missRegion.stride = mSBT.GetGroupsStride();
			missRegion.size = mSBT.GetMissGroupsSize();

			VkStridedDeviceAddressRegionKHR hitRegion{};
			hitRegion.deviceAddress = mSBT.GetAddress() + mSBT.GetHitGroupsOffset();
			hitRegion.stride = mSBT.GetGroupsStride();
			hitRegion.size = mSBT.GetHitGroupsSize();

			VkStridedDeviceAddressRegionKHR callableRegion{};
			App::Instance().GetGraphicsContext()->GetDevice()->vkCmdTraceRaysKHR(mCommandBuffers[i]->GetHandle(), &rayGenRegion, &missRegion, &hitRegion, &callableRegion, App::Instance().GetWindow()->GetExtent().x, App::Instance().GetWindow()->GetExtent().y, 1);

			mCommandBuffers[i]->ImageBarrier(App::Instance().GetGraphicsContext()->GetSwapChain()->GetImages()[i], Access::NONE, Access::TRANSFER_WRITE, ImageLayout::UNDEFINED, ImageLayout::TRANSFER_DST_OPTIMAL, subresourceRange);
			mCommandBuffers[i]->ImageBarrier(mOffscreenImage.GetImage(), Access::SHADER_WRITE, Access::TRANSFER_READ, ImageLayout::GENERAL, ImageLayout::TRANSFER_SRC_OPTIMAL, subresourceRange);

			VkImageCopy copyRegion;
			copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			copyRegion.srcOffset = { 0, 0, 0 };
			copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			copyRegion.dstOffset = { 0, 0, 0 };
			copyRegion.extent = { (uint32_t)App::Instance().GetWindow()->GetExtent().x, (uint32_t)App::Instance().GetWindow()->GetExtent().y, 1 };

			vkCmdCopyImage(mCommandBuffers[i]->GetHandle(),
				mOffscreenImage.GetImage(),
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				App::Instance().GetGraphicsContext()->GetSwapChain()->GetImages()[i],
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&copyRegion);


			mCommandBuffers[i]->ImageBarrier(App::Instance().GetGraphicsContext()->GetSwapChain()->GetImages()[i], Access::TRANSFER_WRITE, Access::NONE, ImageLayout::TRANSFER_DST_OPTIMAL, ImageLayout::PRESENT_SRC_KHR, subresourceRange); });
	}
}

void RtxScene::LoadSceneGeometry()
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn, error;

	std::string fileName = std::string(RAY_TRACING_EXAMPLE_DIR) + "fake_whitted.obj";
	std::string baseDir = std::string(RAY_TRACING_EXAMPLE_DIR);

	bool result = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &error, fileName.c_str(), baseDir.c_str());

	if (!result)
	{
		std::cout << "failed to obj model" << std::endl;
		exit(1);
	}

	mScene.meshes.resize(shapes.size());
	mScene.materials.resize(materials.size());

	for (size_t meshIdx = 0; meshIdx < shapes.size(); ++meshIdx)
	{
		_Mesh &mesh = mScene.meshes[meshIdx];
		const tinyobj::shape_t &shape = shapes[meshIdx];

		const size_t numFaces = shape.mesh.num_face_vertices.size();
		const size_t numVertices = numFaces * 3;

		mesh.numVertices = numVertices;
		mesh.numFaces = numFaces;

		const size_t positionBufferSize = numVertices * sizeof(Vector3f);
		const size_t indexBufferSize = numFaces * 3 * sizeof(uint32_t);
		const size_t facesBufferSize = numFaces * 4 * sizeof(uint32_t);
		const size_t attribsBufferSize = numVertices * sizeof(VertexAttribute);
		const size_t materialIdBufferSize = numFaces * sizeof(uint32_t);

		VK_CHECK(mesh.positionBuffer.Create(positionBufferSize,
											VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
											VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
		VK_CHECK(mesh.indexBuffer.Create(indexBufferSize,
										 VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
										 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
		VK_CHECK(mesh.faceBuffer.Create(facesBufferSize,
										VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
										VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

		VK_CHECK(mesh.attributeBuffer.Create(attribsBufferSize,
											 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
											 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

		VK_CHECK(mesh.materialIDBuffer.Create(materialIdBufferSize,
											  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
											  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

		Vector3f *positions = reinterpret_cast<Vector3f *>(mesh.positionBuffer.Map());
		VertexAttribute *attribs = reinterpret_cast<VertexAttribute *>(mesh.attributeBuffer.Map());
		uint32_t *indices = reinterpret_cast<uint32_t *>(mesh.indexBuffer.Map());
		uint32_t *faces = reinterpret_cast<uint32_t *>(mesh.faceBuffer.Map());
		uint32_t *matIDs = reinterpret_cast<uint32_t *>(mesh.materialIDBuffer.Map());

		size_t vIdx = 0;
		for (size_t f = 0; f < numFaces; ++f)
		{
			assert(shape.mesh.num_face_vertices[f] == 3);
			for (size_t j = 0; j < 3; ++j, ++vIdx)
			{
				const tinyobj::index_t &i = shape.mesh.indices[vIdx];

				Vector3f &pos = positions[vIdx];
				Vector4f &normal = attribs[vIdx].normal;
				Vector4f &uv = attribs[vIdx].uv;

				pos.x = attrib.vertices[3 * i.vertex_index + 0];
				pos.y = attrib.vertices[3 * i.vertex_index + 1];
				pos.z = attrib.vertices[3 * i.vertex_index + 2];

				normal.x = attrib.normals[3 * i.normal_index + 0];
				normal.y = attrib.normals[3 * i.normal_index + 1];
				normal.z = attrib.normals[3 * i.normal_index + 2];

				uv.x = attrib.texcoords[2 * i.texcoord_index + 0];
				uv.y = attrib.texcoords[2 * i.texcoord_index + 1];
			}

			const uint32_t a = static_cast<uint32_t>(3 * f + 0);
			const uint32_t b = static_cast<uint32_t>(3 * f + 1);
			const uint32_t c = static_cast<uint32_t>(3 * f + 2);
			indices[a] = a;
			indices[b] = b;
			indices[c] = c;
			faces[4 * f + 0] = a;
			faces[4 * f + 1] = b;
			faces[4 * f + 2] = c;

			matIDs[f] = static_cast<uint32_t>(shape.mesh.material_ids[f]);
		}

		mesh.materialIDBuffer.Unmap();
		mesh.indexBuffer.Unmap();
		mesh.faceBuffer.Unmap();
		mesh.attributeBuffer.Unmap();
		mesh.positionBuffer.Unmap();
	}

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

	for (size_t i = 0; i < materials.size(); ++i)
	{
		const tinyobj::material_t &srcMat = materials[i];
		_Material &dstMat = mScene.materials[i];

		std::string fullTexturePath = std::string(RAY_TRACING_EXAMPLE_DIR) + "/" + srcMat.diffuse_texname;
		if (dstMat.albedo.Load(fullTexturePath))
		{
			dstMat.albedo.CreateImageView(VK_IMAGE_VIEW_TYPE_2D, dstMat.albedo.GetFormat(), subresourceRange);
			dstMat.albedo.CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
		}
	}

	const size_t numMeshes = mScene.meshes.size();
	const size_t numMaterials = mScene.materials.size();

	mScene.materialIDBufferInfos.resize(numMeshes);
	mScene.attributeBufferInfos.resize(numMeshes);
	mScene.faceBufferInfos.resize(numMeshes);

	for (size_t i = 0; i < numMeshes; ++i)
	{
		const _Mesh &mesh = mScene.meshes[i];
		VkDescriptorBufferInfo &matIDsInfo = mScene.materialIDBufferInfos[i];
		VkDescriptorBufferInfo &attribsInfo = mScene.attributeBufferInfos[i];
		VkDescriptorBufferInfo &facesInfo = mScene.faceBufferInfos[i];

		matIDsInfo.buffer = mesh.materialIDBuffer.GetBuffer();
		matIDsInfo.offset = 0;
		matIDsInfo.range = mesh.materialIDBuffer.GetSize();

		attribsInfo.buffer = mesh.attributeBuffer.GetBuffer();
		attribsInfo.offset = 0;
		attribsInfo.range = mesh.attributeBuffer.GetSize();

		facesInfo.buffer = mesh.faceBuffer.GetBuffer();
		facesInfo.offset = 0;
		facesInfo.range = mesh.faceBuffer.GetSize();
	}

	mScene.textureImageInfos.resize(numMaterials);
	for (size_t i = 0; i < numMaterials; ++i)
	{
		const _Material &mat = mScene.materials[i];
		VkDescriptorImageInfo &textureInfo = mScene.textureImageInfos[i];

		textureInfo.sampler = mat.albedo.GetSampler();
		textureInfo.imageView = mat.albedo.GetImageView();
		textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}
}
void RtxScene::CreateScene()
{
	mScene.BuildBLAS(App::Instance().GetGraphicsContext()->GetDevice());
	mScene.BuildTLAS(App::Instance().GetGraphicsContext()->GetDevice());

	mEnvTexture.Load(std::string(RAY_TRACING_EXAMPLE_DIR) + "studio_garden_2k.jpg");

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

	mEnvTexture.CreateImageView(VK_IMAGE_VIEW_TYPE_2D, mEnvTexture.GetFormat(), subresourceRange);
	mEnvTexture.CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);

	mEnvTextureDescInfo.sampler = mEnvTexture.GetSampler();
	mEnvTextureDescInfo.imageView = mEnvTexture.GetImageView();
	mEnvTextureDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}
void RtxScene::CreateCamera()
{
	VK_CHECK(mCameraBuffer.Create(sizeof(UniformParams), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

	mCamera.SetViewport(0, App::Instance().GetWindow()->GetExtent().x, 0, App::Instance().GetWindow()->GetExtent().y);
	mCamera.SetViewPlanes(0.1f, 100.0f);
	mCamera.SetFovY(45.0f);
	mCamera.LookAt(Vector3f(0.25f, 3.20f, 6.15f), Vector3f(0.25f, 2.75f, 5.25f));
}
void RtxScene::UpdateCameraParams(UniformParams *params, const float dt)
{
	Vector2f moveDelta(0.0f, 0.0f);

	const uint8_t *keyboardState = SDL_GetKeyboardState(nullptr);
	if (keyboardState[SDL_SCANCODE_W])
		moveDelta.y += 1.0f;
	if (keyboardState[SDL_SCANCODE_S])
		moveDelta.y -= 1.0f;

	if (keyboardState[SDL_SCANCODE_A])
		moveDelta.x -= 1.0f;

	if (keyboardState[SDL_SCANCODE_D])
		moveDelta.x += 1.0f;

	bool isBoost = false;
	if (keyboardState[SDL_SCANCODE_LSHIFT])
		isBoost = true;

	moveDelta *= moveSpeed * dt * (isBoost ? accelMult : 1.0f);

	mCamera.Move(moveDelta.x, moveDelta.y);

	SDL_SetRelativeMouseMode(SDL_TRUE);
	Vector2i32 p;
	SDL_GetRelativeMouseState(&p.x, &p.y);

	mCamera.Rotate(-p.x * rotateSpeed * dt, -p.y * rotateSpeed * dt);

	params->camPos = Vector4f(mCamera.GetPosition(), 1.0f);
	params->camDir = Vector4f(mCamera.GetDirection(), 0.0f);
	params->camUp = Vector4f(mCamera.GetUp(), 0.0f);
	params->camSide = Vector4f(mCamera.GetSide(), 0.0f);
	params->camNearFarFov = Vector4f(mCamera.GetNearPlane(), mCamera.GetFarPlane(), Math::ToRadian(mCamera.GetFovY()), 0.0f);
}
void RtxScene::CreateDescriptorSetLayouts()
{
	const uint32_t numMeshes = mScene.meshes.size();
	const uint32_t numMaterials = mScene.materials.size();

	mDescriptorSetsLayouts.resize(3);

	// rgen Set:
	// binding 0 ->tlas
	// binding 1 ->outputImage
	// binding 2 ->cameraData
	VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding;
	accelerationStructureLayoutBinding.binding = 0;
	accelerationStructureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	accelerationStructureLayoutBinding.descriptorCount = 1;
	accelerationStructureLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	accelerationStructureLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding resultImageLayoutBinding;
	resultImageLayoutBinding.binding = 1;
	resultImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	resultImageLayoutBinding.descriptorCount = 1;
	resultImageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	resultImageLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding cameraDataBufferBinding;
	cameraDataBufferBinding.binding = 2;
	cameraDataBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	cameraDataBufferBinding.descriptorCount = 1;
	cameraDataBufferBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	cameraDataBufferBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> bindings = {
		accelerationStructureLayoutBinding,
		resultImageLayoutBinding,
		cameraDataBufferBinding};

	VkDescriptorSetLayoutCreateInfo set0LayoutInfo;
	set0LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	set0LayoutInfo.pNext = nullptr;
	set0LayoutInfo.flags = 0;
	set0LayoutInfo.bindingCount = bindings.size();
	set0LayoutInfo.pBindings = bindings.data();

	VK_CHECK(vkCreateDescriptorSetLayout(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), &set0LayoutInfo, nullptr, &mDescriptorSetsLayouts[0]));

	// rchit set:
	// binding 0-N ->per-face material ID for meshes(N=num meshes)
	// binding 0-N ->vertexattributes meshes (N=num meshes)
	// binding 0-N -> faces info (indices) for meshes (N=num meshes)
	// binding 0-N ->textures (N= num materials)

	VkDescriptorSetLayoutBinding materialSsboBinding;
	materialSsboBinding.binding = 0;
	materialSsboBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	materialSsboBinding.descriptorCount = numMeshes;
	materialSsboBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
	materialSsboBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding vertexAttributeSsboBinding;
	vertexAttributeSsboBinding.binding = 1;
	vertexAttributeSsboBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	vertexAttributeSsboBinding.descriptorCount = numMeshes;
	vertexAttributeSsboBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
	vertexAttributeSsboBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding faceSsboBinding;
	faceSsboBinding.binding = 2;
	faceSsboBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	faceSsboBinding.descriptorCount = numMeshes;
	faceSsboBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
	faceSsboBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding textureSsboBinding;
	textureSsboBinding.binding = 3;
	textureSsboBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textureSsboBinding.descriptorCount = numMaterials;
	textureSsboBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
	textureSsboBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> set1Bindings = {
		materialSsboBinding,
		vertexAttributeSsboBinding,
		faceSsboBinding,
		textureSsboBinding};

	VkDescriptorSetLayoutCreateInfo set1LayoutInfo;
	set1LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	set1LayoutInfo.pNext = nullptr;
	set1LayoutInfo.flags = 0;
	set1LayoutInfo.bindingCount = set1Bindings.size();
	set1LayoutInfo.pBindings = set1Bindings.data();

	VK_CHECK(vkCreateDescriptorSetLayout(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), &set1LayoutInfo, nullptr, &mDescriptorSetsLayouts[1]));

	// rmiss set:
	// binding 0->env texture
	VkDescriptorSetLayoutBinding envBinding;
	envBinding.binding = 0;
	envBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	envBinding.descriptorCount = 1;
	envBinding.stageFlags = VK_SHADER_STAGE_MISS_BIT_KHR;
	envBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo set2LayoutInfo;
	set2LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	set2LayoutInfo.pNext = nullptr;
	set2LayoutInfo.flags = 0;
	set2LayoutInfo.bindingCount = 1;
	set2LayoutInfo.pBindings = &envBinding;

	VK_CHECK(vkCreateDescriptorSetLayout(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), &set2LayoutInfo, nullptr, &mDescriptorSetsLayouts[2]));
}
void RtxScene::CreateRayTracingPipelineAndSBT()
{
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = nullptr;
	pipelineLayoutCreateInfo.flags = 0;
	pipelineLayoutCreateInfo.setLayoutCount = mDescriptorSetsLayouts.size();
	pipelineLayoutCreateInfo.pSetLayouts = mDescriptorSetsLayouts.data();
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	VK_CHECK(vkCreatePipelineLayout(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), &pipelineLayoutCreateInfo, nullptr, &mPipelineLayout));

	VkUtils::Shader rayGenShader, rayChitShader, rayMissShader;
	rayGenShader.LoadFromFile("shader.rgen.spv");
	rayChitShader.LoadFromFile("shader.rchit.spv");
	rayMissShader.LoadFromFile("shader.rmiss.spv");

	mSBT.Init(1, 1, App::Instance().GetGraphicsContext()->GetDevice()->GetRayTracingPipelineProps().shaderGroupHandleSize, App::Instance().GetGraphicsContext()->GetDevice()->GetRayTracingPipelineProps().shaderGroupBaseAlignment);
	mSBT.SetRayGenStage(rayGenShader.GetShaderStage(VK_SHADER_STAGE_RAYGEN_BIT_KHR));
	mSBT.AddStageToHitGroup({rayChitShader.GetShaderStage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)}, 0);
	mSBT.AddStageToMissGroup({rayMissShader.GetShaderStage(VK_SHADER_STAGE_MISS_BIT_KHR)}, 0);

	VkRayTracingPipelineCreateInfoKHR rayPipelineInfo{};
	rayPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	rayPipelineInfo.stageCount = mSBT.GetNumStages();
	rayPipelineInfo.pStages = mSBT.GetStages();
	rayPipelineInfo.groupCount = mSBT.GetNumGroups();
	rayPipelineInfo.pGroups = mSBT.GetGroups();
	rayPipelineInfo.maxPipelineRayRecursionDepth = 1;
	rayPipelineInfo.layout = mPipelineLayout;

	VK_CHECK(App::Instance().GetGraphicsContext()->GetDevice()->vkCreateRayTracingPipelinesKHR(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayPipelineInfo, nullptr, &mPipeline));

	mSBT.Create(App::Instance().GetGraphicsContext()->GetDevice(), mPipeline);
}
void RtxScene::UpdateDescriptorSets()
{
	const uint32_t numMeshes = mScene.meshes.size();
	const uint32_t numMaterials = mScene.materials.size();

	std::vector<VkDescriptorPoolSize> poolSizes({
		{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1},		   // tlas
		{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},					   // output image
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},					   // camera data
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, numMeshes * 3},		   // per-face material IDs for each mesh
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, numMaterials}, // textures for each material
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}			   // env texture
	});
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.pNext = nullptr;
	descriptorPoolCreateInfo.flags = 0;
	descriptorPoolCreateInfo.maxSets = 6;
	descriptorPoolCreateInfo.poolSizeCount = poolSizes.size();
	descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();

	VK_CHECK(vkCreateDescriptorPool(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), &descriptorPoolCreateInfo, nullptr, &mDescriptorPool));

	mDescriptorSets.resize(3);

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.pNext = nullptr;
	descriptorSetAllocateInfo.descriptorPool = mDescriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = mDescriptorSetsLayouts.size();
	descriptorSetAllocateInfo.pSetLayouts = mDescriptorSetsLayouts.data();

	VK_CHECK(vkAllocateDescriptorSets(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), &descriptorSetAllocateInfo, mDescriptorSets.data()));

	VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo;
	descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	descriptorAccelerationStructureInfo.pNext = nullptr;
	descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
	descriptorAccelerationStructureInfo.pAccelerationStructures = &mScene.tlas.accelerationStructure;

	VkWriteDescriptorSet accelerationStructureWrite;
	accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
	accelerationStructureWrite.dstSet = mDescriptorSets[0];
	accelerationStructureWrite.dstBinding = 0;
	accelerationStructureWrite.dstArrayElement = 0;
	accelerationStructureWrite.descriptorCount = 1;
	accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	accelerationStructureWrite.pImageInfo = nullptr;
	accelerationStructureWrite.pBufferInfo = nullptr;
	accelerationStructureWrite.pTexelBufferView = nullptr;

	VkDescriptorImageInfo descriptorOutputImageInfo;
	descriptorOutputImageInfo.sampler = VK_NULL_HANDLE;
	descriptorOutputImageInfo.imageView = mOffscreenImage.GetImageView();
	descriptorOutputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	VkWriteDescriptorSet resultImageWrite;
	resultImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	resultImageWrite.pNext = nullptr;
	resultImageWrite.dstSet = mDescriptorSets[0];
	resultImageWrite.dstBinding = 1;
	resultImageWrite.dstArrayElement = 0;
	resultImageWrite.descriptorCount = 1;
	resultImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	resultImageWrite.pImageInfo = &descriptorOutputImageInfo;
	resultImageWrite.pBufferInfo = nullptr;
	resultImageWrite.pTexelBufferView = nullptr;

	VkDescriptorBufferInfo cameraDataBufferInfo;
	cameraDataBufferInfo.buffer = mCameraBuffer.GetBuffer();
	cameraDataBufferInfo.offset = 0;
	cameraDataBufferInfo.range = mCameraBuffer.GetSize();

	VkWriteDescriptorSet cameraDataBufferWrite;
	cameraDataBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	cameraDataBufferWrite.pNext = nullptr;
	cameraDataBufferWrite.dstSet = mDescriptorSets[0];
	cameraDataBufferWrite.dstBinding = 2;
	cameraDataBufferWrite.dstArrayElement = 0;
	cameraDataBufferWrite.descriptorCount = 1;
	cameraDataBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	cameraDataBufferWrite.pImageInfo = nullptr;
	cameraDataBufferWrite.pBufferInfo = &cameraDataBufferInfo;
	cameraDataBufferWrite.pTexelBufferView = nullptr;

	VkWriteDescriptorSet matIDBufferWrite;
	matIDBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	matIDBufferWrite.pNext = nullptr;
	matIDBufferWrite.dstSet = mDescriptorSets[1];
	matIDBufferWrite.dstBinding = 0;
	matIDBufferWrite.dstArrayElement = 0;
	matIDBufferWrite.descriptorCount = numMeshes;
	matIDBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	matIDBufferWrite.pImageInfo = nullptr;
	matIDBufferWrite.pBufferInfo = mScene.materialIDBufferInfos.data();
	matIDBufferWrite.pTexelBufferView = nullptr;

	VkWriteDescriptorSet attribBufferWrite;
	attribBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	attribBufferWrite.pNext = nullptr;
	attribBufferWrite.dstSet = mDescriptorSets[1];
	attribBufferWrite.dstBinding = 1;
	attribBufferWrite.dstArrayElement = 0;
	attribBufferWrite.descriptorCount = numMeshes;
	attribBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	attribBufferWrite.pImageInfo = nullptr;
	attribBufferWrite.pBufferInfo = mScene.attributeBufferInfos.data();
	attribBufferWrite.pTexelBufferView = nullptr;

	VkWriteDescriptorSet facesBufferWrite;
	facesBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	facesBufferWrite.pNext = nullptr;
	facesBufferWrite.dstSet = mDescriptorSets[1];
	facesBufferWrite.dstBinding = 2;
	facesBufferWrite.dstArrayElement = 0;
	facesBufferWrite.descriptorCount = numMeshes;
	facesBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	facesBufferWrite.pImageInfo = nullptr;
	facesBufferWrite.pBufferInfo = mScene.faceBufferInfos.data();
	facesBufferWrite.pTexelBufferView = nullptr;

	VkWriteDescriptorSet textureBufferWrite;
	textureBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	textureBufferWrite.pNext = nullptr;
	textureBufferWrite.dstSet = mDescriptorSets[1];
	textureBufferWrite.dstBinding = 3;
	textureBufferWrite.dstArrayElement = 0;
	textureBufferWrite.descriptorCount = numMaterials;
	textureBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textureBufferWrite.pImageInfo = mScene.textureImageInfos.data();
	textureBufferWrite.pBufferInfo = nullptr;
	textureBufferWrite.pTexelBufferView = nullptr;

	VkWriteDescriptorSet envTexturesWrite;
	envTexturesWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	envTexturesWrite.pNext = nullptr;
	envTexturesWrite.dstSet = mDescriptorSets[2];
	envTexturesWrite.dstBinding = 0;
	envTexturesWrite.dstArrayElement = 0;
	envTexturesWrite.descriptorCount = 1;
	envTexturesWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	envTexturesWrite.pImageInfo = &mEnvTextureDescInfo;
	envTexturesWrite.pBufferInfo = nullptr;
	envTexturesWrite.pTexelBufferView = nullptr;

	std::vector<VkWriteDescriptorSet> descriptorWrites({accelerationStructureWrite,
														resultImageWrite,
														cameraDataBufferWrite,
														matIDBufferWrite,
														attribBufferWrite,
														facesBufferWrite,
														textureBufferWrite,
														envTexturesWrite});

	vkUpdateDescriptorSets(App::Instance().GetGraphicsContext()->GetDevice()->GetHandle(), descriptorWrites.size(), descriptorWrites.data(), 0, VK_NULL_HANDLE);
}