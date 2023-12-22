#include "SceneMandelbrotSetGen.h"
#include "labgraphics.h"
#include <stb/stb_image_write.h>
#include <iostream>
#include "Base/App.h"
void SceneMandelbrotSetGen::Init()
{
    mImgBuffer = App::Instance().GetGraphicsContext()->GetDevice()->CreateCPUStorageBuffer(sizeof(Vector4f) * WIDTH * HEIGHT);

    mComputeDescriptorTable = std::make_unique<DescriptorTable>(*App::Instance().GetGraphicsContext()->GetDevice());
    mComputeDescriptorTable->AddLayoutBinding(0, 1, DescriptorType::STORAGE_BUFFER, ShaderStage::COMPUTE);

    mComputeDescriptorSet = mComputeDescriptorTable->AllocateDescriptorSet();
    mComputeDescriptorSet->WriteBuffer(0, mImgBuffer.get(), 0, mImgBuffer->GetSize()).Update();

    mComputePipelineLayout = std::make_unique<PipelineLayout>(*App::Instance().GetGraphicsContext()->GetDevice());
    mComputePipelineLayout->AddDescriptorSetLayout(mComputeDescriptorTable->GetLayout());

    mComputePipeline = std::make_unique<ComputePipeline>(*App::Instance().GetGraphicsContext()->GetDevice());
    mComputePipeline->SetShader(App::Instance().GetGraphicsContext()->GetDevice()->CreateShader(ShaderStage::COMPUTE, ReadFile(std::string(ASSETS_DIR) + "shaders/mandelbrot-set.comp")))
        .SetPipelineLayout(mComputePipelineLayout.get());

    mComputeCommandBuffer = App::Instance().GetGraphicsContext()->GetDevice()->GetComputeCommandPool()->CreatePrimaryCommandBuffer();
}

void SceneMandelbrotSetGen::Render()
{
    mComputeCommandBuffer->ExecuteImmediately([&]()
                                              {
		mComputeCommandBuffer->BindDescriptorSets( mComputePipelineLayout.get(), 0, {mComputeDescriptorSet});
		mComputeCommandBuffer->BindPipeline(mComputePipeline.get());
		mComputeCommandBuffer->Dispatch((uint32_t)ceil(WIDTH / float(WORKGROUP_SIZE)), (uint32_t)ceil(HEIGHT / float(WORKGROUP_SIZE)), 1);
         });
        
    SaveImage("mandelbrot.png");
    App::Instance().Quit();
}

void SceneMandelbrotSetGen::SaveImage(std::string_view fileName)
{
    Vector4f *pixels = mImgBuffer->Map<Vector4f>(0, mImgBuffer->GetSize());

    std::vector<uint8_t> image(WIDTH*HEIGHT*4);

    for (size_t i = 0; i < WIDTH * HEIGHT; ++i)
    {
        image[i*4+0]=((uint8_t)(255.0f * pixels[i].x));
        image[i*4+1]=((uint8_t)(255.0f * pixels[i].y));
        image[i*4+2]=((uint8_t)(255.0f * pixels[i].z));
        image[i*4+3]=((uint8_t)(255.0f * pixels[i].w));
    }

    mImgBuffer->Unmap();

    stbi_write_png(fileName.data(), WIDTH, HEIGHT, 4, image.data(), WIDTH * 4);
}