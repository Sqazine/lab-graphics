#include "SceneMandelbrotSetGen.h"
#include "labgraphics.h"
#include <stb/stb_image_write.h>
#include <iostream>
#include "Base/App.h"
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
                                                  mComputeCommandBuffer->Dispatch((uint32_t)ceil(mWindowExtent.x / float(WORKGROUP_SIZE)), (uint32_t)ceil(mWindowExtent.y / float(WORKGROUP_SIZE)), 1);
                                              });

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

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
        .SetPipelineLayout(mPipelineLayout.get());

    mRasterPipeline->pColorBlendState = colorBlendStateInfo;
    mRasterPipeline->renderPass = App::Instance().GetGraphicsContext()->GetSwapChain()->GetDefaultRenderPass()->GetHandle();

    mRasterCommandBuffers = App::Instance().GetGraphicsContext()->GetDevice()->GetRasterCommandPool()->CreatePrimaryCommandBuffers(App::Instance().GetGraphicsContext()->GetSwapChain()->GetDefaultFrameBuffers().size());

    mImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    mRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    mInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    mImagesInFlight.resize(App::Instance().GetGraphicsContext()->GetSwapChain()->GetImages().size(), nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        mImageAvailableSemaphores[i] = App::Instance().GetGraphicsContext()->GetDevice()->CreateSemaphore();
        mRenderFinishedSemaphores[i] = App::Instance().GetGraphicsContext()->GetDevice()->CreateSemaphore();
        mInFlightFences[i] = App::Instance().GetGraphicsContext()->GetDevice()->CreateFence(FenceStatus::SIGNALED);
    }

    for (size_t i = 0; i < mRasterCommandBuffers.size(); ++i)
    {
        mRasterCommandBuffers[i]->Record([&]()
                                         {
                                             mRasterCommandBuffers[i]->BeginRenderPass(App::Instance().GetGraphicsContext()->GetSwapChain()->GetDefaultRenderPass()->GetHandle(),
                                                                                       App::Instance().GetGraphicsContext()->GetSwapChain()->GetDefaultFrameBuffers()[i]->GetHandle(),
                                                                                       App::Instance().GetGraphicsContext()->GetSwapChain()->GetRenderArea(),
                                                                                       {VkClearValue{0.0f, 0.0f, 0.0f, 1.0f}},
                                                                                       VK_SUBPASS_CONTENTS_INLINE);
                                             mRasterCommandBuffers[i]->BindDescriptorSets(mPipelineLayout.get(), 0, {mDescriptorSet});
                                             mRasterCommandBuffers[i]->SetViewport(mRasterPipeline->GetViewport(0));
                                             mRasterCommandBuffers[i]->BindPipeline(mRasterPipeline.get());
                                             mRasterCommandBuffers[i]->Draw(3, 1, 0, 0);
                                             mRasterCommandBuffers[i]->EndRenderPass();
                                         });
    }
}

void SceneMandelbrotSetGen::Update()
{
}

void SceneMandelbrotSetGen::Render()
{
    uint32_t swapChainImageIdx = App::Instance().GetGraphicsContext()->GetSwapChain()->AcquireNextImage(mImageAvailableSemaphores[currentFrame].get());

    if (mImagesInFlight[swapChainImageIdx] != nullptr)
        mImagesInFlight[swapChainImageIdx]->Wait(VK_TRUE, UINT64_MAX);
    mImagesInFlight[swapChainImageIdx] = mInFlightFences[currentFrame].get();

    mInFlightFences[currentFrame]->Reset();

    mRasterCommandBuffers[swapChainImageIdx]->Submit({PipelineStage::COLOR_ATTACHMENT_OUTPUT}, {mImageAvailableSemaphores[currentFrame].get()}, {mRenderFinishedSemaphores[currentFrame].get()}, mInFlightFences[currentFrame].get());
    mRasterCommandBuffers[swapChainImageIdx]->Present(swapChainImageIdx, {mRenderFinishedSemaphores[currentFrame].get()});
    App::Instance().GetGraphicsContext()->GetDevice()->GetPresentQueue()->WaitIdle();

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

// void SceneMandelbrotSetGen::SaveImage(std::string_view fileName)
// {
//     Vector4f *pixels = mImgBuffer->Map<Vector4f>(0, mImgBuffer->GetSize());

//     std::vector<uint8_t> image(mWindowExtent.x * mWindowExtent.y * 4);

//     for (size_t i = 0; i < mWindowExtent.x * mWindowExtent.y; ++i)
//     {
//         image[i * 4 + 0] = ((uint8_t)(255.0f * pixels[i].x));
//         image[i * 4 + 1] = ((uint8_t)(255.0f * pixels[i].y));
//         image[i * 4 + 2] = ((uint8_t)(255.0f * pixels[i].z));
//         image[i * 4 + 3] = ((uint8_t)(255.0f * pixels[i].w));
//     }

//     mImgBuffer->Unmap();

//     stbi_write_png(fileName.data(), mWindowExtent.x, mWindowExtent.y, 4, image.data(), mWindowExtent.x * 4);
// }