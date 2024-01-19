#pragma once

#include <vulkan/vulkan.h>
#include <SDL_vulkan.h>
#include <vector>
#include "Mesh.h"
#include "Image.h"
#include "VK/Instance.h"
#include "VK/Device.h"

constexpr int32_t LIGHT_NUM = 3;
struct PbrLight
{
	alignas(16) Vector3f direction;
	alignas(16) Vector3f radiance;
	alignas(4) bool enable;
};


struct TransformUniforms
{
    Matrix4f projectionMatrix;
    Matrix4f viewMatrix;
    Matrix4f modelMatrix;
    Matrix4f skyboxRotationMatrix;
};

struct ShadingUniforms
{
    PbrLight lights[LIGHT_NUM];
    Vector4f eyePosition;
};

struct SpecularFilterPushConstants
{
    uint32_t level;
    float roughness;
};

template <typename T>
struct Resource
{
    T handle;
    VkDeviceMemory memory;
    VkDeviceSize allocateSize;
    uint32_t memoryTypeIndex;
};

struct MeshBuffer
{
    Resource<VkBuffer> vertexBuffer;
    Resource<VkBuffer> indexBuffer;
    uint32_t numElements;
};

using ModelBuffer = std::vector<MeshBuffer>;

struct PbrTexture
{
    Resource<VkImage> image;
    VkImageView view;
    uint32_t width, height;
    uint32_t layers;
    uint32_t levels;
};

struct RenderTarget
{
    Resource<VkImage> colorImage;
    Resource<VkImage> depthImage;
    VkImageView colorView;
    VkImageView depthView;
    VkFormat colorFormat;
    VkFormat depthFormat;
    uint32_t width, height;
    uint32_t samples;
};

struct UniformBufferWrap
{
    Resource<VkBuffer> buffer;
    VkDeviceSize capacity;
    VkDeviceSize cursor;
    void *hostMemoryPtr;
};

struct UniformBufferWrapAllocation
{
    VkDescriptorBufferInfo descriptorInfo;
    void *hostMemoryPtr;

    template <typename T>
    T *as() const
    {
        return reinterpret_cast<T *>(hostMemoryPtr);
    }
};

struct ImageMemoryBarrier
{
    ImageMemoryBarrier(const PbrTexture &texture, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        barrier.srcAccessMask = srcAccessMask;
        barrier.dstAccessMask = dstAccessMask;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = texture.image.handle;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    }

    operator VkImageMemoryBarrier() const
    {
        return barrier;
    }

    ImageMemoryBarrier &AspectMask(VkImageAspectFlags aspectMask)
    {
        barrier.subresourceRange.aspectMask = aspectMask;
        return *this;
    }

    ImageMemoryBarrier &MipLevels(uint32_t baseMipLevel, uint32_t levelCount = VK_REMAINING_MIP_LEVELS)
    {
        barrier.subresourceRange.baseMipLevel = baseMipLevel;
        barrier.subresourceRange.levelCount = levelCount;

        return *this;
    }

    ImageMemoryBarrier &ArrayLayers(uint32_t baseArrayLayer, uint32_t layerCount = VK_REMAINING_ARRAY_LAYERS)
    {
        barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
        barrier.subresourceRange.layerCount = layerCount;
        return *this;
    }

    VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
};

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void Init(uint32_t maxSamples);

    void Render(const class PbrScene &scene);

private:
    Resource<VkBuffer> CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags) const;
    Resource<VkImage> CreateImage(uint32_t width, uint32_t height, uint32_t layers, uint32_t levels, VkFormat format, uint32_t samples, VkImageUsageFlags usage) const;

    void DestroyBuffer(Resource<VkBuffer> &buffer) const;
    void DestroyImage(Resource<VkImage> &image) const;

    ModelBuffer CreateModelBuffer(const PbrModel &model) const;
    void DestroyModelBuffer(ModelBuffer &model) const;

    MeshBuffer CreateMeshBuffer(const PbrMesh &mesh) const;
    void DestroyMeshBuffer(MeshBuffer &meshBuffer) const;

    PbrTexture CreateTexture(uint32_t width, uint32_t height, uint32_t layers, VkFormat format, uint32_t levels = 0, VkImageUsageFlags additionUsage = 0) const;
    PbrTexture CreateTexture(const Image &image, VkFormat format, uint32_t levels = 0) const;
    VkImageView CreateTextureView(const PbrTexture &texture, VkFormat format, VkImageAspectFlags aspectMask, uint32_t baseMipLevel, uint32_t numMipLevels) const;
    void GenerateMipmaps(const PbrTexture &texture) const;
    void DestroyTexture(PbrTexture &texture) const;

    RenderTarget CreateRenderTarget(uint32_t width, uint32_t height, uint32_t samples, VkFormat colorFormat, VkFormat depthFormat) const;
    void DestroyRenderTarget(RenderTarget &rt) const;

    UniformBufferWrap CreateUniformBuffer(VkDeviceSize capacity) const;
    void DestroyUniformBuffer(UniformBufferWrap &uniformBuffer) const;

    UniformBufferWrapAllocation AllocFromUniformBuffer(UniformBufferWrap &buffer, VkDeviceSize size) const;
    template <typename T>
    UniformBufferWrapAllocation AllocFromUniformBuffer(UniformBufferWrap &buffer) const
    {
        return AllocFromUniformBuffer(buffer, sizeof(T));
    }

    VkDescriptorSet AllocateDescriptorSet(VkDescriptorPool pool, VkDescriptorSetLayout layout) const;
    void UpdateDescriptorSet(VkDescriptorSet dstSet, uint32_t dstBinding, VkDescriptorType descriptorType, const std::vector<VkDescriptorImageInfo> &descriptors) const;
    void UpdateDescriptorSet(VkDescriptorSet dstSet, uint32_t dstBinding, VkDescriptorType descriptorType, const std::vector<VkDescriptorBufferInfo> &descriptors) const;

    VkDescriptorSetLayout CreateDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding> *bindings = nullptr) const;
    VkPipelineLayout CreatePipelineLayout(const std::vector<VkDescriptorSetLayout> *setLayouts = nullptr, const std::vector<VkPushConstantRange> *pushConstants = nullptr) const;

    VkPipeline CreateGraphicsPipeline(uint32_t subpass, const std::vector<uint32_t> &vs, const std::vector<uint32_t> &fs, VkPipelineLayout layout,
                                      const std::vector<VkVertexInputBindingDescription> *vertexInputBindings = nullptr,
                                      const std::vector<VkVertexInputAttributeDescription> *vertexAttributes = nullptr,
                                      const VkPipelineMultisampleStateCreateInfo *multisampleState = nullptr,
                                      const VkPipelineDepthStencilStateCreateInfo *depthStencilState = nullptr) const;

    VkPipeline CreateComputePipeline(const std::vector<uint32_t> &cs, VkPipelineLayout layout, const VkSpecializationInfo *specializationInfo = nullptr) const;

    VkShaderModule CreateShaderModuleFromSPV(const std::vector<uint32_t> &spvCode) const;

    VkCommandBuffer BeginImmediateCommandBuffer() const;

    void ExecuteImmediateCommandBuffer(VkCommandBuffer commandBuffer) const;

    void CopyToDevice(VkDeviceMemory deviceMemory, const void *data, size_t size) const;

    void PipelineBarrier(VkCommandBuffer commandBuffer,
                         VkPipelineStageFlags srcStageMask,
                         VkPipelineStageFlags dstStageMask,
                         const std::vector<ImageMemoryBarrier> &barriers) const;

    void PresentFrame();

    uint32_t QueryRenderTargetFormatMaxSamples(VkFormat format, VkImageUsageFlags usage) const;
    uint32_t ChooseMemoryType(const VkMemoryRequirements &memoryRequirements, VkMemoryPropertyFlags perferredFlags, VkMemoryPropertyFlags requiredFlags = 0) const;
    bool MemoryTypeNeedsStaging(uint32_t memoryTypeIndex) const;

    VkDescriptorPool mDescriptorPool;

    VkRenderPass mRenderPass;

    VkDescriptorSet mPbrDescriptorSet;
    VkPipelineLayout mPbrPipelineLayout;
    VkPipeline mPbrPipeline;

    VkDescriptorSet mSkyboxDescriptorSet;
    VkPipelineLayout mSkyboxPipelineLayout;
    VkPipeline mSkyboxPipeline;

    std::vector<VkDescriptorSet> mToneMapDescriptorSets;
    VkPipelineLayout mToneMapPipelineLayout;
    VkPipeline mToneMapPipeline;

    VkSampler mDefaultSampler;
    VkSampler mSpecularBRDFSampler;

    uint32_t mNumFrames;
    std::vector<VkFramebuffer> mFramebuffers;
    std::vector<std::unique_ptr<RasterCommandBuffer>> mRasterCommandBuffers;
    std::vector<VkFence> mSubmitFences;
    std::vector<RenderTarget> mRenderTargets;
    std::vector<RenderTarget> mResolveRenderTargets;

    VkFence mPresentationFence;

    uint32_t mRenderSamples;
    VkRect2D mFrameRect;
    uint32_t mFrameIndex;
    uint32_t mFrameCount;

    UniformBufferWrap mUniformBuffer;
    std::vector<UniformBufferWrapAllocation> mTransformUniforms;
    std::vector<UniformBufferWrapAllocation> mShadingUniforms;
    std::vector<VkDescriptorSet> mUniformsDescriptorSets;

    ModelBuffer mPbrModelBuffer;
    ModelBuffer mSkyboxModelBuffer;

    PbrTexture mAlbedoTexture;
    PbrTexture mNormalTexture;
    PbrTexture mMetalnessTexture;
    PbrTexture mRoughnessTexture;
    PbrTexture mEnvTexture;
    PbrTexture mIrradianceTexture;
    PbrTexture mBrdfLut;
};