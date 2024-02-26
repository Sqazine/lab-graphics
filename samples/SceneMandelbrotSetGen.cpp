#include "SceneMandelbrotSetGen.h"
#include "labgraphics.h"
#include <stb/stb_image_write.h>
#include <iostream>
#include "App.h"
void SceneMandelbrotSetGen::Init()
{
    mWindowExtent = App::Instance().GetWindow()->GetExtent();

    mUniform.width = mWindowExtent.x;
    mUniform.height = mWindowExtent.y;

    mComputeImgBuffer = App::Instance().GetGraphicsContext()->GetDevice()->CreateGPUStorageBuffer(sizeof(Vector4f) * mWindowExtent.x * mWindowExtent.y);

    mUniformBuffer = App::Instance().GetGraphicsContext()->GetDevice()->CreateUniformBuffer<Uniform>();
    mUniformBuffer->Set(mUniform);

    mDescriptorTable = std::make_unique<DescriptorTable>(*App::Instance().GetGraphicsContext()->GetDevice());
    mDescriptorTable->AddLayoutBinding(0, 1, DescriptorType::STORAGE_BUFFER, ShaderStage::COMPUTE | ShaderStage::FRAGMENT)
        .AddLayoutBinding(1, 1, DescriptorType::UNIFORM_BUFFER, ShaderStage::COMPUTE | ShaderStage::FRAGMENT);

    mDescriptorSet = mDescriptorTable->AllocateDescriptorSet();
    mDescriptorSet->WriteBuffer(0, mComputeImgBuffer.get())
        .WriteBuffer(1, mUniformBuffer.get())
        .Update();

    mPipelineLayout = std::make_unique<PipelineLayout>(*App::Instance().GetGraphicsContext()->GetDevice());
    mPipelineLayout->AddDescriptorSetLayout(mDescriptorTable->GetLayout());

    mComputePipeline = std::make_unique<ComputePipeline>(*App::Instance().GetGraphicsContext()->GetDevice());
    mComputePipeline->SetShader(App::Instance().GetGraphicsContext()->GetDevice()->CreateShader(ShaderStage::COMPUTE, ReadFile(std::string(ASSETS_DIR) + "shaders/mandelbrot-set.comp")))
        .SetPipelineLayout(mPipelineLayout.get());

    mComputeCommandBuffer = App::Instance().GetGraphicsContext()->GetDevice()->GetComputeCommandPool()->CreatePrimaryCommandBuffer();

    mUniformBuffer->Set(mUniform);
    mComputeCommandBuffer->ExecuteImmediately([&]()
                                              {
                                                  mComputeCommandBuffer->BindDescriptorSets(mPipelineLayout.get(), 0, {mDescriptorSet});
                                                  mComputeCommandBuffer->BindPipeline(mComputePipeline.get());
                                                  mComputeCommandBuffer->Dispatch((uint32_t)ceil(mWindowExtent.x / float(WORKGROUP_SIZE)), (uint32_t)ceil(mWindowExtent.y / float(WORKGROUP_SIZE)), 1); });

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = BLEND_FACTOR_CAST(BlendFactor::ONE);
    colorBlendAttachment.dstColorBlendFactor = BLEND_FACTOR_CAST(BlendFactor::ZERO);
    colorBlendAttachment.colorBlendOp = BLEND_OP_CAST(BlendOp::ADD);
    colorBlendAttachment.srcAlphaBlendFactor = BLEND_FACTOR_CAST(BlendFactor::ZERO);
    colorBlendAttachment.dstAlphaBlendFactor = BLEND_FACTOR_CAST(BlendFactor::ONE);
    colorBlendAttachment.alphaBlendOp = BLEND_OP_CAST(BlendOp::ADD);
    colorBlendAttachment.colorWriteMask = COLOR_COMPONENT_CAST(ColorComponent::ALL);

    VkPipelineColorBlendStateCreateInfo colorBlendStateInfo = {};
    colorBlendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateInfo.pNext = nullptr;
    colorBlendStateInfo.flags = 0;
    colorBlendStateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendStateInfo.attachmentCount = 1;
    colorBlendStateInfo.pAttachments = &colorBlendAttachment;
    colorBlendStateInfo.blendConstants[0] = 0.0f;
    colorBlendStateInfo.blendConstants[1] = 0.0f;
    colorBlendStateInfo.blendConstants[2] = 0.0f;
    colorBlendStateInfo.blendConstants[3] = 0.0f;

    auto vertShader = App::Instance().GetGraphicsContext()->GetDevice()->CreateShader(ShaderStage::VERTEX, ReadFile(std::string(ASSETS_DIR) + "shaders/post.vert"));
    auto fragShader = App::Instance().GetGraphicsContext()->GetDevice()->CreateShader(ShaderStage::FRAGMENT, ReadFile(std::string(ASSETS_DIR) + "shaders/post.frag"));

    mRasterPipeline = std::make_unique<RasterPipeline>(*App::Instance().GetGraphicsContext()->GetDevice());
    mRasterPipeline->SetVertexShader(vertShader)
        .SetFragmentShader(fragShader)
        .SetPrimitiveTopology(PrimitiveTopology::TRIANGLE_LIST)
        .SetPrimitiveRestartEnable(false)
        .AddViewport(Vector2f::ZERO, App::Instance().GetGraphicsContext()->GetSwapChain()->GetExtent())
        .AddScissor(Vector2i32::ZERO, App::Instance().GetGraphicsContext()->GetSwapChain()->GetExtent())
        .SetPolygonMode(PolygonMode::FILL)
        .SetFrontFace(FrontFace::CCW)
        .SetPipelineLayout(mPipelineLayout.get())
        .SetRenderPass(App::Instance().GetGraphicsContext()->GetSwapChain()->GetDefaultRenderPass());

    mRasterPipeline->pColorBlendState = colorBlendStateInfo;

    mRasterPass = std::make_unique<RasterPass>(App::Instance().GetGraphicsContext()->GetSwapChain()->GetImages().size());
    mRasterPass->RecordCommand([&](RasterCommandBuffer *rasterCmd, uint32_t frameIdx)
                               {
                                   rasterCmd->BeginRenderPass(App::Instance().GetGraphicsContext()->GetSwapChain()->GetDefaultRenderPass()->GetHandle(),
                                                              App::Instance().GetGraphicsContext()->GetSwapChain()->GetDefaultFrameBuffers()[frameIdx]->GetHandle(),
                                                              App::Instance().GetGraphicsContext()->GetSwapChain()->GetRenderArea(),
                                                              {VkClearValue{0.0f, 0.0f, 0.0f, 1.0f}},
                                                              VK_SUBPASS_CONTENTS_INLINE);
                                   rasterCmd->BindDescriptorSets(mPipelineLayout.get(), 0, {mDescriptorSet});
                                   rasterCmd->SetViewport(mRasterPipeline->GetViewport(0));
                                   rasterCmd->BindPipeline(mRasterPipeline.get());
                                   rasterCmd->Draw(3, 1, 0, 0);
                                   rasterCmd->EndRenderPass(); });
}

void SceneMandelbrotSetGen::Update()
{
}

void SceneMandelbrotSetGen::Render()
{
    mRasterPass->Render();
}