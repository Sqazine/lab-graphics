#include "SceneSph.h"
#include <iostream>

SceneSph::SceneSph()
{
}

SceneSph::~SceneSph()
{
}

void SceneSph::Init()
{
	ColorAttachment colorAttachment0;
	colorAttachment0.SetBlendDesc(false);

	mRasterPipelineLayout = std::make_unique<PipelineLayout>(*App::Instance().GetGraphicsContext()->GetDevice());

	auto vertShader = App::Instance().GetGraphicsContext()->GetDevice()->CreateShader(ShaderStage::VERTEX, ReadFile(std::string(ASSETS_DIR) + "shaders/sph_particle.vert"));
	auto fragShader = App::Instance().GetGraphicsContext()->GetDevice()->CreateShader(ShaderStage::FRAGMENT, ReadFile(std::string(ASSETS_DIR) + "shaders/sph_particle.frag"));

	mRasterPipeline = std::make_unique<RasterPipeline>(*App::Instance().GetGraphicsContext()->GetDevice());

	mRasterPipeline->SetVertexShader(vertShader)
		.SetFragmentShader(fragShader)
		.AddVertexInputBinding(0, sizeof(Vector2f))
		.AddVertexInputAttribute(0, 0, 0, Format::R32G32_SFLOAT)
		.SetPrimitiveTopology(PrimitiveTopology::POINT_LIST)
		.SetPrimitiveRestartEnable(false)
		.AddViewport(Vector2f::ZERO, App::Instance().GetGraphicsContext()->GetSwapChain()->GetExtent())
		.AddScissor(Vector2i32::ZERO, App::Instance().GetGraphicsContext()->GetSwapChain()->GetExtent())
		.SetPolygonMode(PolygonMode::FILL)
		.SetFrontFace(FrontFace::CCW)
		.SetPipelineLayout(mRasterPipelineLayout.get())
		.SetRenderPass(App::Instance().GetGraphicsContext()->GetSwapChain()->GetDefaultRenderPass())
		.SetColorAttachment(0, colorAttachment0);

	auto frameCount = App::Instance().GetGraphicsContext()->GetSwapChain()->GetImages().size();

	mSphRasterPass = std::make_unique<RasterPass>(frameCount);

	mComputeDescriptorTable = std::make_unique<DescriptorTable>(*App::Instance().GetGraphicsContext()->GetDevice());
	mComputeDescriptorTable->AddLayoutBinding(0, 1, DescriptorType::STORAGE_BUFFER, ShaderStage::COMPUTE);
	mComputeDescriptorTable->AddLayoutBinding(1, 1, DescriptorType::STORAGE_BUFFER, ShaderStage::COMPUTE);
	mComputeDescriptorTable->AddLayoutBinding(2, 1, DescriptorType::STORAGE_BUFFER, ShaderStage::COMPUTE);
	mComputeDescriptorTable->AddLayoutBinding(3, 1, DescriptorType::STORAGE_BUFFER, ShaderStage::COMPUTE);
	mComputeDescriptorTable->AddLayoutBinding(4, 1, DescriptorType::STORAGE_BUFFER, ShaderStage::COMPUTE);

	mComputePipelineLayout = std::make_unique<PipelineLayout>(*App::Instance().GetGraphicsContext()->GetDevice());
	mComputePipelineLayout->AddDescriptorSetLayout(mComputeDescriptorTable->GetLayout());

	mComputePipelines[0] = std::make_unique<ComputePipeline>(*App::Instance().GetGraphicsContext()->GetDevice());
	mComputePipelines[0]->SetShader(App::Instance().GetGraphicsContext()->GetDevice()->CreateShader(ShaderStage::COMPUTE, ReadFile(std::string(ASSETS_DIR) + "shaders/sph_density_pressure.comp"))).SetPipelineLayout(mComputePipelineLayout.get());

	mComputePipelines[1] = std::make_unique<ComputePipeline>(*App::Instance().GetGraphicsContext()->GetDevice());
	mComputePipelines[1]->SetShader(App::Instance().GetGraphicsContext()->GetDevice()->CreateShader(ShaderStage::COMPUTE, ReadFile(std::string(ASSETS_DIR) + "shaders/sph_force.comp"))).SetPipelineLayout(mComputePipelineLayout.get());

	mComputePipelines[2] = std::make_unique<ComputePipeline>(*App::Instance().GetGraphicsContext()->GetDevice());
	mComputePipelines[2]->SetShader(App::Instance().GetGraphicsContext()->GetDevice()->CreateShader(ShaderStage::COMPUTE, ReadFile(std::string(ASSETS_DIR) + "shaders/sph_integrate.comp"))).SetPipelineLayout(mComputePipelineLayout.get());

	mComputeCommandBuffer = App::Instance().GetGraphicsContext()->GetDevice()->GetComputeCommandPool()->CreatePrimaryCommandBuffer();

	mPositionBuffer = App::Instance().GetGraphicsContext()->GetDevice()->CreateGPUBuffer(mPosSsboSize, BufferUsage::VERTEX | BufferUsage::STORAGE | BufferUsage::TRANSFER_DST);
	mVelocityBuffer = App::Instance().GetGraphicsContext()->GetDevice()->CreateGPUBuffer(mVelocitySsboSize, BufferUsage::STORAGE | BufferUsage::TRANSFER_DST);
	mForceBuffer = App::Instance().GetGraphicsContext()->GetDevice()->CreateGPUBuffer(mForceSsboSize, BufferUsage::STORAGE | BufferUsage::TRANSFER_DST);
	mDensityBuffer = App::Instance().GetGraphicsContext()->GetDevice()->CreateGPUBuffer(mDensitySsboSize, BufferUsage::STORAGE | BufferUsage::TRANSFER_DST);
	mPressureBuffer = App::Instance().GetGraphicsContext()->GetDevice()->CreateGPUBuffer(mPressureSsboSize, BufferUsage::STORAGE | BufferUsage::TRANSFER_DST);

	mComputeDescriptorSet = mComputeDescriptorTable->AllocateDescriptorSet();
	mComputeDescriptorSet->WriteBuffer(0, mPositionBuffer.get());
	mComputeDescriptorSet->WriteBuffer(1, mVelocityBuffer.get());
	mComputeDescriptorSet->WriteBuffer(2, mForceBuffer.get());
	mComputeDescriptorSet->WriteBuffer(3, mDensityBuffer.get());
	mComputeDescriptorSet->WriteBuffer(4, mPressureBuffer.get()).Update();

	std::array<Vector2f, PARTICLE_NUM> initParticlePosition;
	for (auto i = 0, x = 0, y = 0; i < PARTICLE_NUM; ++i)
	{
		initParticlePosition[i].x = -0.625f + PARTICLE_RADIUS * 2 * x;
		initParticlePosition[i].y = -1 + PARTICLE_RADIUS * 2 * y;
		x++;
		if (x >= 125)
		{
			x = 0;
			y++;
		}
	}

	InitParticleData(initParticlePosition);

	mSphRasterPass->RecordAllCommands([&](RasterCommandBuffer *rasterCmd, size_t frameIdx)
									  {
	
											rasterCmd->BeginRenderPass(App::Instance().GetGraphicsContext()->GetSwapChain()->GetDefaultRenderPass()->GetHandle(),
																					   App::Instance().GetGraphicsContext()->GetSwapChain()->GetDefaultFrameBuffers()[frameIdx]->GetHandle(),
																					   App::Instance().GetGraphicsContext()->GetSwapChain()->GetRenderArea(),
																					   {VkClearValue{0.0f, 0.0f, 0.0f, 1.0f}},
																					   VK_SUBPASS_CONTENTS_INLINE);

											 rasterCmd->SetViewport(mRasterPipeline->GetViewport(0));
											 rasterCmd->SetScissor(mRasterPipeline->GetScissor(0));
											 rasterCmd->BindPipeline(mRasterPipeline.get());
											 rasterCmd->BindVertexBuffers(0, 1, {mPositionBuffer.get()});
											 rasterCmd->Draw(PARTICLE_NUM, 1, 0, 0);
											 rasterCmd->EndRenderPass(); });

	mComputeCommandBuffer->Record([&]()
								  {
									  mComputeCommandBuffer->BindPipeline(mComputePipelines[0].get());
									  mComputeCommandBuffer->BindDescriptorSets(mComputePipelines[0]->GetLayout(), 0, {mComputeDescriptorSet});
									  mComputeCommandBuffer->Dispatch(WORK_GROUP_NUM, 1, 1);
									  mComputeCommandBuffer->PipelineBarrier(PipelineStage::COMPUTE_SHADER, PipelineStage::COMPUTE_SHADER, 0, 0, nullptr, 0, nullptr, 0, nullptr);

									  mComputeCommandBuffer->BindPipeline(mComputePipelines[1].get());
									  mComputeCommandBuffer->BindDescriptorSets(mComputePipelines[1]->GetLayout(), 0, {mComputeDescriptorSet});
									  mComputeCommandBuffer->Dispatch(WORK_GROUP_NUM, 1, 1);
									  mComputeCommandBuffer->PipelineBarrier(PipelineStage::COMPUTE_SHADER, PipelineStage::COMPUTE_SHADER, 0, 0, nullptr, 0, nullptr, 0, nullptr);

									  mComputeCommandBuffer->BindPipeline(mComputePipelines[2].get());
									  mComputeCommandBuffer->BindDescriptorSets(mComputePipelines[2]->GetLayout(), 0, {mComputeDescriptorSet});
									  mComputeCommandBuffer->Dispatch(WORK_GROUP_NUM, 1, 1);
									  mComputeCommandBuffer->PipelineBarrier(PipelineStage::COMPUTE_SHADER, PipelineStage::COMPUTE_SHADER, 0, 0, nullptr, 0, nullptr, 0, nullptr); });
}

void SceneSph::Update()
{
	auto &inputSystem = App::Instance().GetInputSystem();
	if (inputSystem.GetKeyboard().GetKeyState(SDL_SCANCODE_1) == ButtonState::PRESS)
	{
		std::array<Vector2f, PARTICLE_NUM> initParticlePosition;
		for (auto i = 0, x = 0, y = 0; i < PARTICLE_NUM; ++i)
		{
			initParticlePosition[i].x = -0.625f + PARTICLE_RADIUS * 2 * x;
			initParticlePosition[i].y = -1 + PARTICLE_RADIUS * 2 * y;
			x++;
			if (x >= 125)
			{
				x = 0;
				y++;
			}
		}
		InitParticleData(initParticlePosition);
	}
	if (inputSystem.GetKeyboard().GetKeyState(SDL_SCANCODE_2) == ButtonState::PRESS)
	{
		std::array<Vector2f, PARTICLE_NUM> initParticlePosition;
		for (auto i = 0, x = 0, y = 0; i < PARTICLE_NUM; ++i)
		{
			initParticlePosition[i].x = -1 + PARTICLE_RADIUS * 2 * x;
			initParticlePosition[i].y = 1 - PARTICLE_RADIUS * 2 * y;
			x++;
			if (x >= 100)
			{
				x = 0;
				y++;
			}
		}
		InitParticleData(initParticlePosition);
	}
	if (inputSystem.GetKeyboard().GetKeyState(SDL_SCANCODE_3) == ButtonState::PRESS)
	{
		std::array<Vector2f, PARTICLE_NUM> initParticlePosition;
		for (auto i = 0, x = 0, y = 0; i < PARTICLE_NUM; ++i)
		{
			initParticlePosition[i].x = 1 - PARTICLE_RADIUS * 2 * x;
			initParticlePosition[i].y = -1 + PARTICLE_RADIUS * 2 * y;
			x++;
			if (x >= 100)
			{
				x = 0;
				y++;
			}
		}
		InitParticleData(initParticlePosition);
	}

	Simulate();
}
void SceneSph::Simulate()
{
	mComputeCommandBuffer->Submit();

	App::Instance().GetGraphicsContext()->GetDevice()->GetComputeQueue()->WaitIdle();
}

void SceneSph::Render()
{
	mSphRasterPass->Render();
}

void SceneSph::InitParticleData(std::array<Vector2f, PARTICLE_NUM> initParticlePosition)
{
	auto stagingBuffer = App::Instance().GetGraphicsContext()->GetDevice()->CreateCPUBuffer(mPosSsboSize, BufferUsage::TRANSFER_SRC);
	stagingBuffer->FillWhole(initParticlePosition.data());
	mPositionBuffer->UploadDataFrom(stagingBuffer->GetSize(), *stagingBuffer);
}