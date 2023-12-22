#pragma once
#include <vulkan/vulkan.h>
#include "VkUtils.h"
#include <vector>
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

struct Material
{
    VkUtils::Image albedo;
};

struct _Scene
{
    std::vector<_Mesh> meshes;
    std::vector<Material> materials;

    AccelerationStructure tlas;

    std::vector<VkDescriptorBufferInfo> materialIDBufferInfos;
    std::vector<VkDescriptorBufferInfo> attributeBufferInfos;
    std::vector<VkDescriptorBufferInfo> faceBufferInfos;
    std::vector<VkDescriptorImageInfo> textureImageInfos;

    void BuildBLAS(Device* device, VkCommandPool commandPool);
    void BuildTLAS(Device* device, VkCommandPool commandPool);
};
