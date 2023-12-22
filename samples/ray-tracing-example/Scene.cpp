#include "Scene.h"
#include <iostream>
#include "labgraphics.h"
void _Scene::BuildBLAS(Device* device, VkCommandPool commandPool)
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

    VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VK_CHECK(vkAllocateCommandBuffers(device->GetHandle(), &commandBufferAllocateInfo, &commandBuffer));

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkMemoryBarrier memoryBarrier{};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.pNext = nullptr;
    memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

    for (size_t i = 0; i < numMeshes; ++i)
    {
        _Mesh &mesh = meshes[i];
        mesh.blas.buffer.Create(sizeInfos[i].accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkAccelerationStructureCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        createInfo.size = sizeInfos[i].accelerationStructureSize;
        createInfo.buffer = mesh.blas.buffer.GetBuffer();

        VK_CHECK(device->vkCreateAccelerationStructureKHR(device->GetHandle(), &createInfo, nullptr, &mesh.blas.accelerationStructure));

        VkAccelerationStructureBuildGeometryInfoKHR &buildInfo = buildInfos[i];
        buildInfo.scratchData = VkUtils::GetBufferDeviceAddress(scratchBuffer);
        buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
        buildInfo.dstAccelerationStructure = mesh.blas.accelerationStructure;

        const VkAccelerationStructureBuildRangeInfoKHR *range[1] = {&ranges[i]};

        device->vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildInfo, range);

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                             VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                             0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
    }

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    device->GetGraphicsQueue()->Submit(submitInfo);
    device->GetGraphicsQueue()->WaitIdle();

    vkFreeCommandBuffers(device->GetHandle(), commandPool, 1, &commandBuffer);

    for (size_t i = 0; i < numMeshes; ++i)
    {
        _Mesh &mesh = meshes[i];

        VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
        addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        addressInfo.accelerationStructure = mesh.blas.accelerationStructure;
        mesh.blas.address = device->vkGetAccelerationStructureDeviceAddressKHR(device->GetHandle(), &addressInfo);
    }
}
void _Scene::BuildTLAS(Device* device, VkCommandPool commandPool)
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

    VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VK_CHECK(vkAllocateCommandBuffers(device->GetHandle(), &commandBufferAllocateInfo, &commandBuffer));

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkAccelerationStructureBuildRangeInfoKHR range = {};
    range.primitiveCount = numInstances;

    const VkAccelerationStructureBuildRangeInfoKHR *ranges[1] = {&range};

    device->vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildInfo, ranges);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    device->GetGraphicsQueue()->Submit(submitInfo);
    device->GetGraphicsQueue()->WaitIdle();

    vkFreeCommandBuffers(device->GetHandle(), commandPool, 1, &commandBuffer);

    VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
    addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    addressInfo.accelerationStructure = tlas.accelerationStructure;
    tlas.address = device->vkGetAccelerationStructureDeviceAddressKHR(device->GetHandle(), &addressInfo);
}
