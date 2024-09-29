#include "RtxRayTraceScene.h"
#include "VK/Buffer.h"
#include "RtxRayTracePass.h"
#include "VK/CommandPool.h"
#include "VK/CommandBuffer.h"
#include "App.h"
#include "App.h"
RtxRayTraceScene::RtxRayTraceScene(RaymanScene *scene)
    : mScene(scene)
{

    mRtxRayTracePass.reset(new RtxRayTracePass(*App::Instance().GetGraphicsContext()->GetInstance(), *App::Instance().GetGraphicsContext()->GetDevice()));
    mRtxRayTracePass->SetScene(this);
}

RtxRayTraceScene::~RtxRayTraceScene()
{
}

void RtxRayTraceScene::CreateBuffers()
{
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    std::vector<Vector2u32> offsets;

    std::vector<Matrix4f> worldMatrixs;

    for (const auto &meshInstance : mScene->meshInstances)
    {
        auto &mesh = mScene->meshes[meshInstance.meshId];

        const auto indexOffset = static_cast<uint32_t>(indices.size());
        const auto vertexOffset = static_cast<uint32_t>(vertices.size());

        Matrix4f modelMatrix = meshInstance.modelTransform;
        Matrix4f modelTransInvMatrix = Matrix4f::Transpose(Matrix4f::Inverse(modelMatrix));

        for (auto &vertex : mesh->vertices)
        {
            vertex.position = Vector4f::ToVector3(modelMatrix * Vector4f(vertex.position, 1.f));
            vertex.normal = Vector3f::Normalize(Vector4f::ToVector3(modelTransInvMatrix * Vector4f(vertex.normal, 1.f)));
            vertex.materialId = meshInstance.materialId;
        }

        offsets.emplace_back(indexOffset, vertexOffset);

        vertices.insert(vertices.end(), mesh->vertices.begin(), mesh->vertices.end());
        indices.insert(indices.end(), mesh->indices.begin(), mesh->indices.end());
        worldMatrixs.emplace_back(meshInstance.modelTransform);
    }

    // =============== VERTEX BUFFER ===============

    auto bufferUsage = BufferUsage::VERTEX | BufferUsage::STORAGE | BufferUsage::SHADER_DEVICE_ADDRESS | BufferUsage::ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY;

    auto size = sizeof(mScene->meshes[0]->vertices[0]) * vertices.size();

    if (size == 0)
    {
        std::cout << "[SCENE ANALYZER] Scene not load!" << std::endl;
        exit(1);
    }

    std::cout << "[SCENE ANALYZER] Vertex buffer size = " << static_cast<double>(size) / 1000000.0 << " MB" << std::endl;
    Fill(mVertexBuffer, vertices.data(), size, bufferUsage);

    // =============== INDEX BUFFER ===============

    bufferUsage = BufferUsage::INDEX | BufferUsage::STORAGE | BufferUsage::SHADER_DEVICE_ADDRESS | BufferUsage::ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY;

    size = sizeof(indices[0]) * indices.size();
    std::cout << "[SCENE ANALYZER] index buffer size = " << static_cast<double>(size) / 1000000.0 << " MB" << std::endl;
    Fill(mIndexBuffer, indices.data(), size, bufferUsage);

    // =============== MATERIAL BUFFER ===============

    size = sizeof(mScene->materials[0]) * mScene->materials.size();
    std::cout << "[SCENE ANALYZER] material buffer size = " << static_cast<double>(size) / 1000000.0 << " MB" << std::endl;
    Fill(mMaterialBuffer, mScene->materials.data(), size, BufferUsage::STORAGE);

    // =============== OFFSET BUFFER ===============

    size = sizeof(offsets[0]) * offsets.size();
    std::cout << "[SCENE ANALYZER] offset buffer size = " << static_cast<double>(size) / 1000000.0 << " MB" << std::endl;
    Fill(mOffsetBuffer, offsets.data(), size, BufferUsage::STORAGE);

    // =============== LIGHTS BUFFER ===============

    size = sizeof(mScene->lights[0]) * mScene->lights.size();
    std::cout << "[SCENE ANALYZER] light buffer size = " << static_cast<double>(size) / 1000000.0 << " MB" << std::endl;
    Fill(mLightsBuffer, mScene->lights.data(), size, BufferUsage::STORAGE);

    auto format = Format::R32G32B32_SFLOAT;
    ImageTiling tiling = ImageTiling::LINEAR;

    mHdrImages.emplace_back(new Texture(*App::Instance().GetGraphicsContext()->GetDevice(), mScene->hdrColumns.get(), format, tiling));
    mHdrImages.emplace_back(new Texture(*App::Instance().GetGraphicsContext()->GetDevice(), mScene->hdrConditional.get(), format, tiling));
    mHdrImages.emplace_back(new Texture(*App::Instance().GetGraphicsContext()->GetDevice(), mScene->hdrMarginal.get(), format, tiling));

    for (const auto &textureData : mScene->textureDatas)
        mTextureImages.emplace_back(new Texture(*App::Instance().GetGraphicsContext()->GetDevice(), textureData.get()));
}

void RtxRayTraceScene::Build()
{
    // Add a dummy texture for texture samplers in Vulkan
    if (mScene->textureDatas.empty())
    {
        mScene->textureDatas.emplace_back(std::make_unique<ImageData>());
        mTextureImages.emplace_back(new Texture(*App::Instance().GetGraphicsContext()->GetDevice(), mScene->textureDatas.back().get()));
    }

    if (mScene->lights.empty())
    {
        Light light;
        light.type = -1;
        mScene->lights.emplace_back(light);
    }

    CreateBuffers();

    mScene->PrintInfo();
}

void RtxRayTraceScene::Fill(
    std::unique_ptr<class GpuBuffer> &buffer,
    void *data,
    size_t size,
    BufferUsage usage) const
{
    auto staging = App::Instance().GetGraphicsContext()->GetDevice()->CreateCPUBuffer(data, size, BufferUsage::TRANSFER_SRC);

    buffer = std::make_unique<GpuBuffer>(*App::Instance().GetGraphicsContext()->GetDevice(), size, BufferUsage::TRANSFER_DST | usage);

    auto cmd = App::Instance().GetGraphicsContext()->GetDevice()->GetTransferCommandPool()->CreatePrimaryCommandBuffer();

    cmd->ExecuteImmediately([&]()
                            {
        	VkBufferCopy copyRegion = {};
						copyRegion.srcOffset = 0;
						copyRegion.dstOffset = 0;
						copyRegion.size = size;

        cmd->CopyBuffer(*buffer,*staging,copyRegion); });
}
void RtxRayTraceScene::ResetAccumulation()
{
    mRtxRayTracePass->ResetAccumulation();
}

void RtxRayTraceScene::SetRenderState(enum RenderState state)
{
    mRtxRayTracePass->SetRenderState(state);
}

void RtxRayTraceScene::SetPostProcessType(PostProcessType type)
{
    mRtxRayTracePass->SetPostProcessType(type);
}

void RtxRayTraceScene::SaveOutputImageToDisk()
{
    mRtxRayTracePass->SaveOutputImageToDisk();
}

void RtxRayTraceScene::Update()
{
    mRtxRayTracePass->Update();
}

void RtxRayTraceScene::Render()
{
    mRtxRayTracePass->Render();
}