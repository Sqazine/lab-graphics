#pragma once
#include <cstdint>
#include <vulkan/vulkan.h>
#include <type_traits>

#define ENUM_CLASS_OP(CastName, EnumName, RawEnum)                   \
    inline EnumName operator|(EnumName lhs, EnumName rhs)            \
    {                                                                \
        return static_cast<EnumName>(CastName(lhs) | CastName(rhs)); \
    }

enum class BufferUsage : uint64_t
{
#define ITEM(x) x = VK_BUFFER_USAGE_##x##_BIT
#define ITEM_KHR(x) x = VK_BUFFER_USAGE_##x##_BIT_KHR
#define ITEM_EXT(x) x = VK_BUFFER_USAGE_##x##_BIT_EXT
#define ITEM_BUFFER(x) x = VK_BUFFER_USAGE_##x##_BUFFER_BIT
#define ITEM_BUFFER_EXT(x) x = VK_BUFFER_USAGE_##x##_BUFFER_BIT_EXT
    NONE = 0,
    ITEM(TRANSFER_SRC),
    ITEM(TRANSFER_DST),
    ITEM_BUFFER(UNIFORM_TEXEL),
    ITEM_BUFFER(STORAGE_TEXEL),
    ITEM_BUFFER(UNIFORM),
    ITEM_BUFFER(STORAGE),
    ITEM_BUFFER(INDEX),
    ITEM_BUFFER(VERTEX),
    ITEM_BUFFER(INDIRECT),
    ITEM(SHADER_DEVICE_ADDRESS),
    ITEM_BUFFER_EXT(TRANSFORM_FEEDBACK),
    ITEM_BUFFER_EXT(TRANSFORM_FEEDBACK_COUNTER),
    ITEM_EXT(CONDITIONAL_RENDERING),
    ITEM_KHR(ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY),
    ITEM_KHR(ACCELERATION_STRUCTURE_STORAGE),
    ITEM_KHR(SHADER_BINDING_TABLE),
    ITEM_BUFFER_EXT(SAMPLER_DESCRIPTOR),
    ITEM_BUFFER_EXT(RESOURCE_DESCRIPTOR),
    ITEM_BUFFER_EXT(PUSH_DESCRIPTORS_DESCRIPTOR),
    ITEM_EXT(MICROMAP_BUILD_INPUT_READ_ONLY),
    ITEM_EXT(MICROMAP_STORAGE),
#undef ITEM
#undef ITEM_KHR
#undef ITEM_EXT
#undef ITEM_BUFFER
#undef ITEM_BUFFER_EXT
};

#define BUFFER_USAGE_CAST(t) (static_cast<VkBufferUsageFlags>(std::underlying_type<BufferUsage>::type(t)))
ENUM_CLASS_OP(BUFFER_USAGE_CAST, BufferUsage, VkBufferUsageFlags)

enum class DescriptorType : uint64_t
{
#define ITEM(x) x = VK_DESCRIPTOR_TYPE_##x
    ITEM(SAMPLER),
    ITEM(COMBINED_IMAGE_SAMPLER),
    ITEM(SAMPLED_IMAGE),
    ITEM(STORAGE_IMAGE),
    ITEM(UNIFORM_TEXEL_BUFFER),
    ITEM(STORAGE_TEXEL_BUFFER),
    ITEM(UNIFORM_BUFFER),
    ITEM(STORAGE_BUFFER),
    ITEM(UNIFORM_BUFFER_DYNAMIC),
    ITEM(STORAGE_BUFFER_DYNAMIC),
    ITEM(INPUT_ATTACHMENT),
    ITEM(INLINE_UNIFORM_BLOCK),
    ITEM(ACCELERATION_STRUCTURE_KHR),
    ITEM(ACCELERATION_STRUCTURE_NV),
    ITEM(SAMPLE_WEIGHT_IMAGE_QCOM),
    ITEM(BLOCK_MATCH_IMAGE_QCOM),
    ITEM(MUTABLE_EXT),
#undef ITEM
};
#define DESCRIPTOR_TYPE_CAST(t) (static_cast<VkDescriptorType>(std::underlying_type<DescriptorType>::type(t)))
ENUM_CLASS_OP(DESCRIPTOR_TYPE_CAST, DescriptorType, VkDescriptorType)

enum class ImageUsage : uint64_t
{
#define ITEM(x) x = VK_IMAGE_USAGE_##x##_BIT
#define ITEM_KHR(x) x = VK_IMAGE_USAGE_##x##_BIT_KHR
    ITEM(TRANSFER_SRC),
    ITEM(TRANSFER_DST),
    ITEM(SAMPLED),
    ITEM(STORAGE),
    ITEM(COLOR_ATTACHMENT),
    ITEM(DEPTH_STENCIL_ATTACHMENT),
    ITEM(TRANSIENT_ATTACHMENT),
    ITEM(INPUT_ATTACHMENT),
    ITEM_KHR(FRAGMENT_SHADING_RATE_ATTACHMENT),
#undef ITEM
#undef ITEM_KHR
};
#define IMAGE_USAGE_CAST(t) (static_cast<VkImageUsageFlags>(std::underlying_type<ImageUsage>::type(t)))
ENUM_CLASS_OP(IMAGE_USAGE_CAST, ImageUsage, VkImageUsageFlags)

enum class ImageLayout : uint64_t
{
#define ITEM(x) x = VK_IMAGE_LAYOUT_##x
    ITEM(UNDEFINED),
    ITEM(GENERAL),
    ITEM(COLOR_ATTACHMENT_OPTIMAL),
    ITEM(DEPTH_STENCIL_ATTACHMENT_OPTIMAL),
    ITEM(DEPTH_STENCIL_READ_ONLY_OPTIMAL),
    ITEM(SHADER_READ_ONLY_OPTIMAL),
    ITEM(TRANSFER_SRC_OPTIMAL),
    ITEM(TRANSFER_DST_OPTIMAL),
    ITEM(PREINITIALIZED),
    ITEM(DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL),
    ITEM(DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL),
    ITEM(DEPTH_ATTACHMENT_OPTIMAL),
    ITEM(DEPTH_READ_ONLY_OPTIMAL),
    ITEM(STENCIL_ATTACHMENT_OPTIMAL),
    ITEM(STENCIL_READ_ONLY_OPTIMAL),
    ITEM(READ_ONLY_OPTIMAL),
    ITEM(ATTACHMENT_OPTIMAL),
    ITEM(PRESENT_SRC_KHR),
#undef ITEM
};

#define IMAGE_LAYOUT_CAST(t) (static_cast<VkImageLayout>(std::underlying_type<ImageLayout>::type(t)))
ENUM_CLASS_OP(IMAGE_LAYOUT_CAST, ImageLayout, VkImageLayout)

enum class ImageAspect : uint64_t
{
#define ITEM(x) x = VK_IMAGE_ASPECT_##x
#define ITEM_BIT(x) x = VK_IMAGE_ASPECT_##x##_BIT
    ITEM(NONE),
    ITEM_BIT(COLOR),
    ITEM_BIT(DEPTH),
    ITEM_BIT(STENCIL),
#undef ITEM
#undef ITEM_BIT
};

#define IMAGE_ASPECT_CAST(imageAspect) (static_cast<VkImageAspectFlags>(std::underlying_type<ImageAspect>::type(imageAspect)))
ENUM_CLASS_OP(IMAGE_ASPECT_CAST, ImageAspect, VkImageAspectFlags)

enum class ImageTiling : uint64_t
{
#define ITEM(x) x = VK_IMAGE_TILING_##x
    ITEM(OPTIMAL),
    ITEM(LINEAR),
#undef ITEM
};
#define IMAGE_TILING_CAST(imageTiling) (static_cast<VkImageTiling>(std::underlying_type<ImageTiling>::type(imageTiling)))
ENUM_CLASS_OP(IMAGE_TILING_CAST, ImageTiling, VkImageTiling)

enum class PipelineStage : uint64_t
{
#define ITEM_NO_POSTFIX(x) VK_PIPELINE_STAGE_##x
#define ITEM(x) x = VK_PIPELINE_STAGE_##x##_BIT
#define ITEM_EXT(x) x = VK_PIPELINE_STAGE_##x##_BIT_EXT
#define ITEM_KHR(x) x = VK_PIPELINE_STAGE_##x##_BIT_KHR

    ITEM_NO_POSTFIX(NONE),
    ITEM(TOP_OF_PIPE),
    ITEM(BOTTOM_OF_PIPE),
    ITEM(DRAW_INDIRECT),
    ITEM(VERTEX_INPUT),

    ITEM(VERTEX_SHADER),
    ITEM(TESSELLATION_CONTROL_SHADER),
    ITEM(TESSELLATION_EVALUATION_SHADER),
    ITEM(GEOMETRY_SHADER),
    ITEM(FRAGMENT_SHADER),
    ITEM(COMPUTE_SHADER),

    ITEM(EARLY_FRAGMENT_TESTS),

    ITEM(LATE_FRAGMENT_TESTS),
    ITEM(COLOR_ATTACHMENT_OUTPUT),

    ITEM(TRANSFER),

    ITEM(HOST),

    ITEM(ALL_GRAPHICS),
    ITEM(ALL_COMMANDS),

    ITEM_EXT(TASK_SHADER),
    ITEM_EXT(MESH_SHADER),
    ITEM_EXT(TRANSFORM_FEEDBACK),
    ITEM_EXT(CONDITIONAL_RENDERING),
    ITEM_EXT(FRAGMENT_DENSITY_PROCESS),

    ITEM_KHR(ACCELERATION_STRUCTURE_BUILD),
    ITEM_KHR(RAY_TRACING_SHADER),
    ITEM_KHR(FRAGMENT_SHADING_RATE_ATTACHMENT),

#undef ITEM_NO_POSTFIX
#undef ITEM
#undef ITEM_EXT
#undef ITEM_KHR
};
#define PIPELINE_STAGE_CAST(pielineStage) (static_cast<VkPipelineStageFlagBits>(std::underlying_type<PipelineStage>::type(pielineStage)))
ENUM_CLASS_OP(PIPELINE_STAGE_CAST, PipelineStage, VkPipelineStageFlagBits)

enum class SampleCount : uint64_t
{
    X1 = VK_SAMPLE_COUNT_1_BIT,
    X2 = VK_SAMPLE_COUNT_2_BIT,
    X4 = VK_SAMPLE_COUNT_4_BIT,
    X8 = VK_SAMPLE_COUNT_8_BIT,
    X16 = VK_SAMPLE_COUNT_16_BIT,
    X32 = VK_SAMPLE_COUNT_32_BIT,
    X64 = VK_SAMPLE_COUNT_64_BIT,
};
#define SAMPLE_COUNT(sampleCount) (static_cast<VkSampleCountFlagBits>(std::underlying_type<SampleCount>::type(sampleCount)))

enum class PrimitiveTopology : uint64_t
{
#define ITEM(x) x = VK_PRIMITIVE_TOPOLOGY_##x

    ITEM(POINT_LIST),
    ITEM(LINE_LIST),
    ITEM(LINE_STRIP),
    ITEM(TRIANGLE_LIST),
    ITEM(TRIANGLE_STRIP),
    ITEM(TRIANGLE_FAN),

    ITEM(LINE_LIST_WITH_ADJACENCY),
    ITEM(LINE_STRIP_WITH_ADJACENCY),
    ITEM(TRIANGLE_LIST_WITH_ADJACENCY),
    ITEM(TRIANGLE_STRIP_WITH_ADJACENCY),

#undef ITEM
};
#define PRIMITIVE_TOPOLOGY_CAST(primitiveTopologyType) (static_cast<VkPrimitiveTopology>(std::underlying_type<PrimitiveTopology>::type(primitiveTopologyType)))

enum class PolygonMode : uint64_t
{
#define ITEM(x) x = VK_POLYGON_MODE_##x
    ITEM(FILL),
    ITEM(LINE),
    ITEM(POINT),
#undef ITEM
};
#define POLYGON_MODE_CAST(polygonMode) (static_cast<VkPolygonMode>(std::underlying_type<PolygonMode>::type(polygonMode)))

enum class FrontFace : uint64_t
{
    CW = VK_FRONT_FACE_CLOCKWISE,
    CCW = VK_FRONT_FACE_COUNTER_CLOCKWISE
};
#define FRONT_FACE_CAST(frontFace) (static_cast<VkFrontFace>(std::underlying_type<FrontFace>::type(frontFace)))

enum class CullMode : uint64_t
{
#define ITEM(x) x = VK_CULL_MODE_##x
#define ITEM_WITH_POFTFIX(x) x = VK_CULL_MODE_##x##_BIT

    ITEM(NONE),
    ITEM_WITH_POFTFIX(FRONT),
    ITEM_WITH_POFTFIX(BACK),
    ITEM(FRONT_AND_BACK),

#undef ITEM
};
#define CULL_MODE_CAST(mode) (static_cast<VkCullModeFlags>(std::underlying_type<CullMode>::type(mode)))

enum class FilterMode : uint64_t
{
#define ITEM(x) x = VK_FILTER_##x
#define ITEM_EXT(x) x = VK_FILTER_##x##_EXT

    ITEM(NEAREST),
    ITEM(LINEAR),
    ITEM_EXT(CUBIC),

#undef ITEM
#undef ITEM_EXT
};
#define FILTER_MODE_CAST(filterMode) (static_cast<VkFilter>(std::underlying_type<FilterMode>::type(filterMode)))
ENUM_CLASS_OP(FILTER_MODE_CAST, FilterMode, VkFilter)

enum class AddressMode : uint64_t
{
#define ITEM(x) x = VK_SAMPLER_ADDRESS_MODE_##x

    ITEM(REPEAT),
    ITEM(MIRRORED_REPEAT),
    ITEM(CLAMP_TO_EDGE),
    ITEM(CLAMP_TO_BORDER),
    ITEM(MIRROR_CLAMP_TO_EDGE),

#undef ITEM
};
#define ADDRESS_MODE_CAST(addressMode) (static_cast<VkSamplerAddressMode>(std::underlying_type<AddressMode>::type(addressMode)))
ENUM_CLASS_OP(ADDRESS_MODE_CAST, AddressMode, VkSamplerAddressMode)

enum class MipMapMode : uint64_t
{
#define ITEM(x) x = VK_SAMPLER_MIPMAP_MODE_##x

    ITEM(NEAREST),
    ITEM(LINEAR),

#undef ITEM
};
#define MIPMAP_MODE_CAST(mipmapMode) (static_cast<VkSamplerMipmapMode>(std::underlying_type<MipMapMode>::type(mipmapMode)))
ENUM_CLASS_OP(MIPMAP_MODE_CAST, MipMapMode, VkSamplerMipmapMode)

enum class BorderColor : uint64_t
{
#define ITEM(x) x = VK_BORDER_COLOR_##x

    ITEM(FLOAT_TRANSPARENT_BLACK),
    ITEM(INT_TRANSPARENT_BLACK),

    ITEM(FLOAT_OPAQUE_BLACK),
    ITEM(INT_OPAQUE_BLACK),

    ITEM(FLOAT_OPAQUE_WHITE),
    ITEM(INT_OPAQUE_WHITE),

#undef ITEM
};
#define BORDER_COLOR_CAST(borderColorMode) (static_cast<VkBorderColor>(std::underlying_type<BorderColor>::type(borderColorMode)))

enum class ShaderStage : uint64_t
{
#define ITEM(x) x = VK_SHADER_STAGE_##x##_BIT
#define ITEM_KHR(x) x = VK_SHADER_STAGE_##x##_BIT_KHR
#define ITEM_EXT(x) x = VK_SHADER_STAGE_##x##_BIT_EXT
#define ITEM_NO_POSTFIX(x) x = VK_SHADER_STAGE_##x
    ITEM(VERTEX),
    ITEM(FRAGMENT),
    ITEM(GEOMETRY),
    ITEM(TESSELLATION_CONTROL),
    ITEM(TESSELLATION_EVALUATION),
    ITEM(COMPUTE),

    ITEM_KHR(RAYGEN),
    ITEM_KHR(CLOSEST_HIT),
    ITEM_KHR(MISS),
    ITEM_KHR(CALLABLE),
    ITEM_KHR(ANY_HIT),
    ITEM_KHR(INTERSECTION),

    ITEM_EXT(TASK),
    ITEM_EXT(MESH),

    ITEM_NO_POSTFIX(ALL),
    ITEM_NO_POSTFIX(ALL_GRAPHICS),
#undef ITEM
#undef ITEM_KHR
#undef ITEM_EXT
#undef ITEM_NO_POSTFIX

};
#define SHADER_STAGE_CAST(shaderStage) (static_cast<VkShaderStageFlagBits>(std::underlying_type<ShaderStage>::type(shaderStage)))
ENUM_CLASS_OP(SHADER_STAGE_CAST, ShaderStage, VkShaderStageFlagBits)

enum class Access : uint64_t
{
#define ITEM(x) x = VK_ACCESS_##x
#define ITEM_BIT(x) x = VK_ACCESS_##x##_BIT
#define ITEM_EXT(x) x = VK_ACCESS_##x##_BIT_EXT
#define ITEM_KHR(x) x = VK_ACCESS_##x##_BIT_KHR
    ITEM(NONE),
    ITEM_BIT(INDIRECT_COMMAND_READ),
    ITEM_BIT(INDEX_READ),
    ITEM_BIT(VERTEX_ATTRIBUTE_READ),
    ITEM_BIT(UNIFORM_READ),
    ITEM_BIT(INPUT_ATTACHMENT_READ),
    ITEM_BIT(SHADER_READ),
    ITEM_BIT(SHADER_WRITE),
    ITEM_BIT(COLOR_ATTACHMENT_READ),
    ITEM_BIT(COLOR_ATTACHMENT_WRITE),
    ITEM_BIT(DEPTH_STENCIL_ATTACHMENT_READ),
    ITEM_BIT(DEPTH_STENCIL_ATTACHMENT_WRITE),
    ITEM_BIT(TRANSFER_READ),
    ITEM_BIT(TRANSFER_WRITE),
    ITEM_BIT(HOST_READ),
    ITEM_BIT(HOST_WRITE),
    ITEM_BIT(MEMORY_READ),
    ITEM_BIT(MEMORY_WRITE),
    ITEM_EXT(TRANSFORM_FEEDBACK_WRITE),
    ITEM_EXT(TRANSFORM_FEEDBACK_COUNTER_READ),
    ITEM_EXT(TRANSFORM_FEEDBACK_COUNTER_WRITE),
    ITEM_EXT(CONDITIONAL_RENDERING_READ),
    ITEM_EXT(COLOR_ATTACHMENT_READ_NONCOHERENT),
    ITEM_KHR(ACCELERATION_STRUCTURE_READ),
    ITEM_KHR(ACCELERATION_STRUCTURE_WRITE),
#undef ITEM
#undef ITEM_BIT
#undef ITEM_EXT
#undef ITEM_KHR
};
#define ACCESS_CAST(access) (static_cast<VkAccessFlagBits>(std::underlying_type<Access>::type(access)))
ENUM_CLASS_OP(ACCESS_CAST, Access, VkAccessFlagBits)

enum class AttachmentLoad : uint64_t
{
#define ITEM(x) x = VK_ATTACHMENT_LOAD_OP_##x
    ITEM(LOAD),
    ITEM(CLEAR),
    ITEM(DONT_CARE),
#undef ITEM
};
#define ATTACHMENT_LOAD_CAST(load) (static_cast<VkAttachmentLoadOp>(std::underlying_type<AttachmentLoad>::type(load)))
ENUM_CLASS_OP(ATTACHMENT_LOAD_CAST, AttachmentLoad, VkAttachmentLoadOp)

enum class AttachmentStore : uint64_t
{
#define ITEM(x) x = VK_ATTACHMENT_STORE_OP_##x
    ITEM(STORE),
    ITEM(DONT_CARE),
#undef ITEM
};
#define ATTACHMENT_STORE_CAST(store) (static_cast<VkAttachmentStoreOp>(std::underlying_type<AttachmentStore>::type(store)))
ENUM_CLASS_OP(ATTACHMENT_STORE_CAST, AttachmentStore, VkAttachmentStoreOp)

enum class BlendFactor : uint64_t
{
#define ITEM(x) x = VK_BLEND_FACTOR_##x
    ITEM(ZERO),
    ITEM(ONE),
    ITEM(SRC_COLOR),
    ITEM(ONE_MINUS_SRC_COLOR),
    ITEM(DST_COLOR),
    ITEM(ONE_MINUS_DST_COLOR),

    ITEM(SRC_ALPHA),
    ITEM(ONE_MINUS_SRC_ALPHA),
    ITEM(DST_ALPHA),
    ITEM(ONE_MINUS_DST_ALPHA),

    ITEM(CONSTANT_COLOR),
    ITEM(ONE_MINUS_CONSTANT_COLOR),

    ITEM(CONSTANT_ALPHA),
    ITEM(ONE_MINUS_CONSTANT_ALPHA),

    ITEM(SRC_ALPHA_SATURATE),

    ITEM(SRC1_COLOR),
    ITEM(ONE_MINUS_SRC1_COLOR),
    ITEM(SRC1_ALPHA),
    ITEM(ONE_MINUS_SRC1_ALPHA),
#undef ITEM
};
#define BLEND_FACTOR_CAST(t) (static_cast<VkBlendFactor>(std::underlying_type<BlendFactor>::type(t)))
ENUM_CLASS_OP(BLEND_FACTOR_CAST, BlendFactor, VkBlendFactor)

enum class BlendOp : uint64_t
{
#define ITEM(x) x = VK_BLEND_OP_##x

    ITEM(ADD),
    ITEM(SUBTRACT),
    ITEM(REVERSE_SUBTRACT),
    ITEM(MIN),
    ITEM(MAX),

#undef ITEM
};
#define BLEND_OP_CAST(t) (static_cast<VkBlendOp>(std::underlying_type<BlendOp>::type(t)))

enum class ColorComponent : uint64_t
{
#define ITEM(x) x = VK_COLOR_COMPONENT_##x##_BIT

    ITEM(R),
    ITEM(G),
    ITEM(B),
    ITEM(A),
    ALL = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT

#undef ITEM
};
#define COLOR_COMPONENT_CAST(t) (static_cast<VkColorComponentFlags>(std::underlying_type<ColorComponent>::type(t)))
ENUM_CLASS_OP(COLOR_COMPONENT_CAST, ColorComponent, VkColorComponentFlags)