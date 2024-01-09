#include "Renderer.h"
#include "labgraphics.h"
#include "Scene.h"
#include <cassert>
#include <map>

void Renderer::Init(uint32_t maxSamples)
{
	std::vector<const char *> instanceLayers;
#if _DEBUG
	instanceLayers.emplace_back("VK_LAYER_KHRONOS_validation");
	instanceLayers.emplace_back("VK_LAYER_LUNARG_monitor");
#endif
	mInstance=std::make_unique<Instance>(App::Instance().GetWindow(),instanceLayers);
	mDevice=std::make_unique<Device>(*mInstance,DeviceFeature::ANISOTROPY_SAMPLER);

	mSwapChain=std::make_unique<SwapChain>(*mDevice);

	const VkFormat colorFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	const VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

	const uint32_t maxColorSamples = QueryRenderTargetFormatMaxSamples(colorFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	const uint32_t maxDepthSamples = QueryRenderTargetFormatMaxSamples(depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

	mRenderSamples = Math::Min(maxSamples, Math::Min(maxColorSamples, maxDepthSamples));

	assert(mRenderSamples >= 1);

	mNumFrames=mSwapChain->GetImageViews().size();

	mRenderTargets.resize(mNumFrames);
	mResolveRenderTargets.resize(mNumFrames);

	for (uint32_t i = 0; i < mNumFrames; ++i)
	{
		mRenderTargets[i] = CreateRenderTarget(App::Instance().GetWindow()->GetExtent().x, App::Instance().GetWindow()->GetExtent().y, mRenderSamples, colorFormat, depthFormat);

		if (mRenderSamples > 1)
			mResolveRenderTargets[i] = CreateRenderTarget(App::Instance().GetWindow()->GetExtent().x, App::Instance().GetWindow()->GetExtent().y, 1, colorFormat, VK_FORMAT_UNDEFINED);
	}

	mCommandBuffers.resize(mNumFrames);

	VkCommandPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolCreateInfo.queueFamilyIndex = mDevice->GetQueueFamilyIndices().graphicsFamily.value();

	VK_CHECK(vkCreateCommandPool(mDevice->GetHandle(), &poolCreateInfo, nullptr, &mCommandPool));

	VkCommandBufferAllocateInfo commandBufferAllocInfo{};
	commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocInfo.pNext = nullptr;
	commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocInfo.commandBufferCount = mNumFrames;
	commandBufferAllocInfo.commandPool = mCommandPool;

	VK_CHECK(vkAllocateCommandBuffers(mDevice->GetHandle(), &commandBufferAllocInfo, mCommandBuffers.data()));

	mSubmitFences.resize(mNumFrames);

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext = nullptr;
	fenceCreateInfo.flags = 0;

	VK_CHECK(vkCreateFence(mDevice->GetHandle(), &fenceCreateInfo, nullptr, &mPresentationFence));
	for (auto &fence : mSubmitFences)
		VK_CHECK(vkCreateFence(mDevice->GetHandle(), &fenceCreateInfo, nullptr, &fence));

	VK_CHECK(vkAcquireNextImageKHR(mDevice->GetHandle(), mSwapChain->GetHandle(), UINT64_MAX, VK_NULL_HANDLE, mPresentationFence, &mFrameIndex));

	vkWaitForFences(mDevice->GetHandle(), 1, &mPresentationFence, VK_TRUE, UINT64_MAX);
	vkResetFences(mDevice->GetHandle(), 1, &mPresentationFence);

	const std::array<VkDescriptorPoolSize, 3> poolSizes = {{
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 16},
		{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 16},
	}};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.pNext = nullptr;
	descriptorPoolCreateInfo.flags = 0;
	descriptorPoolCreateInfo.maxSets = 16;
	descriptorPoolCreateInfo.poolSizeCount = poolSizes.size();
	descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();

	VK_CHECK(vkCreateDescriptorPool(mDevice->GetHandle(), &descriptorPoolCreateInfo, nullptr, &mDescriptorPool));

	mFrameRect = {0, 0, (uint32_t)App::Instance().GetWindow()->GetExtent().x, (uint32_t)App::Instance().GetWindow()->GetExtent().y};
	mFrameCount = 0;

	constexpr uint32_t kEnvMapSize = 1024;
	constexpr uint32_t kIrradianceMapSize = 32;
	constexpr uint32_t kBRDF_LUT_Size = 256;
	constexpr uint32_t kEnvMapLevels = Math::NumMipmapLevels(kEnvMapSize, kEnvMapSize);
	constexpr VkDeviceSize kUniformBufferSize = 64 * 1024;

	struct
	{
		VkDescriptorSetLayout uniforms;
		VkDescriptorSetLayout pbr;
		VkDescriptorSetLayout skybox;
		VkDescriptorSetLayout tonemap;
		VkDescriptorSetLayout compute;
	} setLayout;

	enum UniformDescriptorSetBindingName
	{
		BINDING_TRANSFORM_UNIFORMS = 0,
		BINDING_SHADING_UNIFORMS = 1
	};

	enum ComputeDescriptorSetBindingName
	{
		BINDING_INPUT_TEXTURE = 0,
		BINDING_OUTPUT_TEXTURE = 1,
		BINDING_OUTPUT_MIP_TAIL = 2,
	};

	mUniformBuffer = CreateUniformBuffer(kUniformBufferSize);

	VkSampler computeSampler;
	{
		VkSamplerCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.minFilter = VK_FILTER_LINEAR;
		createInfo.magFilter = VK_FILTER_LINEAR;
		createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		VK_CHECK(vkCreateSampler(mDevice->GetHandle(), &createInfo, nullptr, &computeSampler));

		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		createInfo.anisotropyEnable = VK_TRUE;
		createInfo.maxAnisotropy = mDevice->GetPhysicalProps().limits.maxSamplerAnisotropy;
		createInfo.minLod = 0.0f;
		createInfo.maxLod = FLT_MAX;

		VK_CHECK(vkCreateSampler(mDevice->GetHandle(), &createInfo, nullptr, &mDefaultSampler));

		createInfo.anisotropyEnable = VK_FALSE;
		createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

		VK_CHECK(vkCreateSampler(mDevice->GetHandle(), &createInfo, nullptr, &mSpecularBRDFSampler));
	}

	VkDescriptorPool computeDescriptorPool;
	{
		const std::array<VkDescriptorPoolSize, 2> poolSizes = {{
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, kEnvMapLevels},
		}};

		VkDescriptorPoolCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.maxSets = 2;
		createInfo.poolSizeCount = (uint32_t)poolSizes.size();
		createInfo.pPoolSizes = poolSizes.data();

		VK_CHECK(vkCreateDescriptorPool(mDevice->GetHandle(), &createInfo, nullptr, &computeDescriptorPool));
	}

	VkPipelineLayout computePipelineLayout;
	VkDescriptorSet computeDescriptorSet;
	{
		const std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
			{BINDING_INPUT_TEXTURE, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, &computeSampler},
			{BINDING_OUTPUT_TEXTURE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
			{BINDING_OUTPUT_MIP_TAIL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, kEnvMapLevels - 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
		};

		setLayout.compute = CreateDescriptorSetLayout(&descriptorSetLayoutBindings);
		computeDescriptorSet = AllocateDescriptorSet(computeDescriptorPool, setLayout.compute);

		const std::vector<VkDescriptorSetLayout> pipelineSetLayouts = {
			setLayout.compute};

		const std::vector<VkPushConstantRange> pipelinePushConstantRanges = {
			{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SpecularFilterPushConstants)},
		};

		computePipelineLayout = CreatePipelineLayout(&pipelineSetLayouts, &pipelinePushConstantRanges);
	}

	{
		const std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
			{BINDING_TRANSFORM_UNIFORMS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{BINDING_SHADING_UNIFORMS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
		};

		setLayout.uniforms = CreateDescriptorSetLayout(&descriptorSetLayoutBindings);
	}

	{
		mUniformsDescriptorSets.resize(mNumFrames);
		for (uint32_t i = 0; i < mNumFrames; ++i)
		{
			mUniformsDescriptorSets[i] = AllocateDescriptorSet(mDescriptorPool, setLayout.uniforms);

			mTransformUniforms.emplace_back(AllocFromUniformBuffer<TransformUniforms>(mUniformBuffer));
			UpdateDescriptorSet(mUniformsDescriptorSets[i], BINDING_TRANSFORM_UNIFORMS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {mTransformUniforms[i].descriptorInfo});

			mShadingUniforms.emplace_back(AllocFromUniformBuffer<ShadingUniforms>(mUniformBuffer));
			UpdateDescriptorSet(mUniformsDescriptorSets[i], BINDING_SHADING_UNIFORMS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {mShadingUniforms[i].descriptorInfo});
		}
	}

	{
		enum AttachmentName
		{
			MAIN_COLOR_ATTACHMENT = 0,
			MAIN_DEPTH_STENCIL_ATTACHMENT,
			SWAP_CHAIN_COLOR_ATTACHMENT,
			RESOLVE_COLOR_ATTACHMENT
		};

		std::vector<VkAttachmentDescription> attachments = {
			{
				0,
				mRenderTargets[0].colorFormat,
				static_cast<VkSampleCountFlagBits>(mRenderSamples),
				VK_ATTACHMENT_LOAD_OP_CLEAR,
				VK_ATTACHMENT_STORE_OP_STORE,
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				VK_ATTACHMENT_STORE_OP_DONT_CARE,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			},
			{
				0,
				mRenderTargets[0].depthFormat,
				static_cast<VkSampleCountFlagBits>(mRenderSamples),
				VK_ATTACHMENT_LOAD_OP_CLEAR,
				VK_ATTACHMENT_STORE_OP_DONT_CARE,
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				VK_ATTACHMENT_STORE_OP_DONT_CARE,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			},
			{
				0,
				VK_FORMAT_B8G8R8A8_UNORM,
				VK_SAMPLE_COUNT_1_BIT,
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				VK_ATTACHMENT_STORE_OP_STORE,
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				VK_ATTACHMENT_STORE_OP_DONT_CARE,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			}};

		if (mRenderSamples > 1)
		{
			const VkAttachmentDescription resolveAttachment = {
				0,
				mResolveRenderTargets[0].colorFormat,
				VK_SAMPLE_COUNT_1_BIT,
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				VK_ATTACHMENT_STORE_OP_STORE,
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				VK_ATTACHMENT_STORE_OP_DONT_CARE,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			};

			attachments.emplace_back(resolveAttachment);
		}

		const std::array<VkAttachmentReference, 1> mainPassColorRefs = {
			{MAIN_COLOR_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
		};

		const std::array<VkAttachmentReference, 1> mainPassResolveRefs = {
			{RESOLVE_COLOR_ATTACHMENT, VK_IMAGE_LAYOUT_GENERAL},
		};

		const VkAttachmentReference mainPassDepthStencilRef = {
			MAIN_DEPTH_STENCIL_ATTACHMENT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

		VkSubpassDescription mainPass{};
		mainPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		mainPass.colorAttachmentCount = (uint32_t)mainPassColorRefs.size();
		mainPass.pColorAttachments = mainPassColorRefs.data();
		mainPass.pDepthStencilAttachment = &mainPassDepthStencilRef;

		if (mRenderSamples > 1)
			mainPass.pResolveAttachments = mainPassResolveRefs.data();

		const std::array<VkAttachmentReference, 1> tonemapPassInputRefs = {
			{MAIN_COLOR_ATTACHMENT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
		};
		const std::array<VkAttachmentReference, 1> tonemapPassMultisampleInputRefs = {
			{RESOLVE_COLOR_ATTACHMENT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
		};
		const std::array<VkAttachmentReference, 1> tonemapPassColorRefs = {
			{SWAP_CHAIN_COLOR_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};

		VkSubpassDescription toneMapPass{};
		toneMapPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		toneMapPass.colorAttachmentCount = tonemapPassColorRefs.size();
		toneMapPass.pColorAttachments = tonemapPassColorRefs.data();

		if (mRenderSamples > 1)
		{
			toneMapPass.inputAttachmentCount = (uint32_t)tonemapPassMultisampleInputRefs.size();
			toneMapPass.pInputAttachments = tonemapPassMultisampleInputRefs.data();
		}
		else
		{
			toneMapPass.inputAttachmentCount = tonemapPassInputRefs.size();
			toneMapPass.pInputAttachments = tonemapPassInputRefs.data();
		}

		const std::array<VkSubpassDescription, 2> subpasses = {
			mainPass,
			toneMapPass,
		};

		const VkSubpassDependency mainToTonemapDependency = {
			0,
			1,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_DEPENDENCY_BY_REGION_BIT,
		};

		VkRenderPassCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = (uint32_t)attachments.size();
		createInfo.pAttachments = attachments.data();
		createInfo.subpassCount = (uint32_t)subpasses.size();
		createInfo.pSubpasses = subpasses.data();
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &mainToTonemapDependency;

		VK_CHECK(vkCreateRenderPass(mDevice->GetHandle(), &createInfo, nullptr, &mRenderPass));
	}

	{
		mFramebuffers.resize(mNumFrames);
		for (uint32_t i = 0; i < mFramebuffers.size(); ++i)
		{
			std::vector<VkImageView> attachments = {
				mRenderTargets[i].colorView,
				mRenderTargets[i].depthView,
				mSwapChain->GetImageViews()[i]->GetHandle(),
			};

			if (mRenderSamples > 1)
				attachments.emplace_back(mResolveRenderTargets[i].colorView);

			VkFramebufferCreateInfo createInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
			createInfo.renderPass = mRenderPass;
			createInfo.attachmentCount = (uint32_t)attachments.size();
			createInfo.pAttachments = attachments.data();
			createInfo.width = mFrameRect.extent.width;
			createInfo.height = mFrameRect.extent.height;
			createInfo.layers = 1;

			VK_CHECK(vkCreateFramebuffer(mDevice->GetHandle(), &createInfo, nullptr, &mFramebuffers[i]));
		}
	}

	{
		mEnvTexture = CreateTexture(kEnvMapSize, kEnvMapSize, 6, VK_FORMAT_R16G16B16A16_SFLOAT, 0, VK_IMAGE_USAGE_STORAGE_BIT);
		mIrradianceTexture = CreateTexture(kIrradianceMapSize, kIrradianceMapSize, 6, VK_FORMAT_R16G16B16A16_SFLOAT, 1, VK_IMAGE_USAGE_STORAGE_BIT);
		mBrdfLut = CreateTexture(kBRDF_LUT_Size, kBRDF_LUT_Size, 1, VK_FORMAT_R16G16_SFLOAT, 1, VK_IMAGE_USAGE_STORAGE_BIT);
	}

	{
		const std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
			{0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
		};

		setLayout.tonemap = CreateDescriptorSetLayout(&descriptorSetLayoutBindings);

		const std::vector<VkDescriptorSetLayout> pipelineDescriptorSetLayouts = {
			setLayout.tonemap};

		mToneMapPipelineLayout = CreatePipelineLayout(&pipelineDescriptorSetLayouts);

		std::vector<uint32_t> tonemap_vs_spv;
		VK_CHECK(GlslToSpv(VK_SHADER_STAGE_VERTEX_BIT, ReadFile(std::string(ASSETS_DIR) + "shaders/tonemap.vert"), tonemap_vs_spv));

		std::vector<uint32_t> tonemap_fs_spv;
		VK_CHECK(GlslToSpv(VK_SHADER_STAGE_FRAGMENT_BIT, ReadFile(std::string(ASSETS_DIR) + "shaders/tonemap.frag"), tonemap_fs_spv));

		mToneMapPipeline = CreateGraphicsPipeline(
			1,
			tonemap_vs_spv,
			tonemap_fs_spv,
			mToneMapPipelineLayout);
	}

	// Allocate & update descriptor sets for tone mapping input (per-frame)
	{
		mToneMapDescriptorSets.resize(mNumFrames);
		for (uint32_t i = 0; i < mNumFrames; ++i)
		{
			VkDescriptorImageInfo imageInfo{};
			imageInfo.sampler = VK_NULL_HANDLE;
			imageInfo.imageView = mRenderSamples > 1 ? mResolveRenderTargets[i].colorView : mRenderTargets[i].colorView;
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			mToneMapDescriptorSets[i] = AllocateDescriptorSet(mDescriptorPool, setLayout.tonemap);
			UpdateDescriptorSet(mToneMapDescriptorSets[i], 0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, {imageInfo});
		}
	}

	mPbrModelBuffer = CreateModelBuffer(_Model(std::string(ASSETS_DIR) + "meshes/cerberus.glb"));

	mAlbedoTexture = CreateTexture(Image(std::string(ASSETS_DIR) + "textures/cerberus_A.png"), VK_FORMAT_R8G8B8A8_SRGB);
	mNormalTexture = CreateTexture(Image(std::string(ASSETS_DIR) + "textures/cerberus_N.png"), VK_FORMAT_R8G8B8A8_UNORM);
	mMetalnessTexture = CreateTexture(Image(std::string(ASSETS_DIR) + "textures/cerberus_M.png"), VK_FORMAT_R8G8B8A8_UNORM);
	mRoughnessTexture = CreateTexture(Image(std::string(ASSETS_DIR) + "textures/cerberus_R.png"), VK_FORMAT_R8G8B8A8_UNORM);

	{
		const std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
			{0, sizeof(_Vertex), VK_VERTEX_INPUT_RATE_VERTEX},
		};

		const std::vector<VkVertexInputAttributeDescription> vertexAttributes = {
			{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},	// Position
			{1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12}, // Normal
			{2, 0, VK_FORMAT_R32G32_SFLOAT, 24},	// Texcoord
		};

		const std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings =
			{
				{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &mDefaultSampler},		// albedo
				{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &mDefaultSampler},		// normal
				{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &mDefaultSampler},		// metalness
				{3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &mDefaultSampler},		// roughness
				{4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &mDefaultSampler},		// specular env
				{5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &mDefaultSampler},		// irradiance
				{6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &mSpecularBRDFSampler}, // brdf
			};

		setLayout.pbr = CreateDescriptorSetLayout(&descriptorSetLayoutBindings);

		const std::vector<VkDescriptorSetLayout> pipelineDescriptorSetLayouts = {
			setLayout.uniforms,
			setLayout.pbr};

		mPbrPipelineLayout = CreatePipelineLayout(&pipelineDescriptorSetLayouts);

		VkPipelineMultisampleStateCreateInfo multiSampleState = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
		multiSampleState.rasterizationSamples = (VkSampleCountFlagBits)mRenderTargets[0].samples;

		VkPipelineDepthStencilStateCreateInfo depthStencilState = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

		std::vector<uint32_t> pbr_vert;
		VK_CHECK(GlslToSpv(VK_SHADER_STAGE_VERTEX_BIT, ReadFile(std::string(ASSETS_DIR) + "shaders/pbr.vert"), pbr_vert));

		std::vector<uint32_t> pbr_frag;
		VK_CHECK(GlslToSpv(VK_SHADER_STAGE_FRAGMENT_BIT, ReadFile(std::string(ASSETS_DIR) + "shaders/pbr.frag"), pbr_frag));

		mPbrPipeline = CreateGraphicsPipeline(0,
											  pbr_vert,
											  pbr_frag,
											  mPbrPipelineLayout,
											  &vertexInputBindings,
											  &vertexAttributes,
											  &multiSampleState,
											  &depthStencilState);
	}

	{
		const std::vector<VkDescriptorImageInfo> textures = {
			{VK_NULL_HANDLE, mAlbedoTexture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
			{VK_NULL_HANDLE, mNormalTexture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
			{VK_NULL_HANDLE, mMetalnessTexture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
			{VK_NULL_HANDLE, mRoughnessTexture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
			{VK_NULL_HANDLE, mEnvTexture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
			{VK_NULL_HANDLE, mIrradianceTexture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
			{VK_NULL_HANDLE, mBrdfLut.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
		};

		mPbrDescriptorSet = AllocateDescriptorSet(mDescriptorPool, setLayout.pbr);
		UpdateDescriptorSet(mPbrDescriptorSet, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, textures);
	}

	mSkyboxModelBuffer = CreateModelBuffer(_Model(_MeshType::CUBE));

	{
		const std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
			{0, sizeof(_Vertex), VK_VERTEX_INPUT_RATE_VERTEX},
		};

		const std::vector<VkVertexInputAttributeDescription> vertexAttribute = {
			{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}};

		const std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
			{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &mDefaultSampler}};

		setLayout.skybox = CreateDescriptorSetLayout(&descriptorSetLayoutBindings);

		const std::vector<VkDescriptorSetLayout> pipelineDescriptorSetLayouts = {
			setLayout.uniforms,
			setLayout.skybox};

		mSkyboxPipelineLayout = CreatePipelineLayout(&pipelineDescriptorSetLayouts);

		VkPipelineMultisampleStateCreateInfo multiSampleState = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
		multiSampleState.rasterizationSamples = static_cast<VkSampleCountFlagBits>(mRenderTargets[0].samples);

		VkPipelineDepthStencilStateCreateInfo depthStencilState = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

		std::vector<uint32_t> skybox_vert;
		VK_CHECK(GlslToSpv(VK_SHADER_STAGE_VERTEX_BIT, ReadFile(std::string(ASSETS_DIR) + "shaders/skybox.vert"), skybox_vert));

		std::vector<uint32_t> skybox_frag;
		VK_CHECK(GlslToSpv(VK_SHADER_STAGE_FRAGMENT_BIT, ReadFile(std::string(ASSETS_DIR) + "shaders/skybox.frag"), skybox_frag));

		mSkyboxPipeline = CreateGraphicsPipeline(0,
												 skybox_vert,
												 skybox_frag,
												 mSkyboxPipelineLayout,
												 &vertexInputBindings,
												 &vertexAttribute,
												 &multiSampleState,
												 &depthStencilState);
	}

	{
		const VkDescriptorImageInfo skyboxTexture = {VK_NULL_HANDLE, mEnvTexture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
		mSkyboxDescriptorSet = AllocateDescriptorSet(mDescriptorPool, setLayout.skybox);
		UpdateDescriptorSet(mSkyboxDescriptorSet, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, {skyboxTexture});
	}

	// Load & pre-process environment map.
	{
		Texture envTexture = CreateTexture(kEnvMapSize, kEnvMapSize, 6, VK_FORMAT_R16G16B16A16_SFLOAT, 0, VK_IMAGE_USAGE_STORAGE_BIT);

		// equirect2cube
		{
			std::vector<uint32_t> equirect2cube_comp;
			VK_CHECK(GlslToSpv(VK_SHADER_STAGE_COMPUTE_BIT, ReadFile(std::string(ASSETS_DIR) + "shaders/equirect2cube.comp"), equirect2cube_comp));

			VkPipeline compPipeline = CreateComputePipeline(equirect2cube_comp, computePipelineLayout);

			Texture envTextureEquirect = CreateTexture(Image(std::string(ASSETS_DIR) + "hdr/newport_loft.hdr"), VK_FORMAT_R32G32B32A32_SFLOAT, 1);

			const VkDescriptorImageInfo inputTexture = {VK_NULL_HANDLE, envTextureEquirect.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
			const VkDescriptorImageInfo outputTexture = {VK_NULL_HANDLE, envTexture.view, VK_IMAGE_LAYOUT_GENERAL};
			UpdateDescriptorSet(computeDescriptorSet, BINDING_INPUT_TEXTURE, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, {inputTexture});
			UpdateDescriptorSet(computeDescriptorSet, BINDING_OUTPUT_TEXTURE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, {outputTexture});

			VkCommandBuffer commandBuffer = BeginImmediateCommandBuffer();

			const auto preDispatchBarrier = ImageMemoryBarrier(envTexture, VK_ACCESS_NONE, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
			PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, {preDispatchBarrier});

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compPipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeDescriptorSet, 0, nullptr);
			vkCmdDispatch(commandBuffer, kEnvMapSize / 32, kEnvMapSize / 32, 6);

			const auto postDispatchBarrier = ImageMemoryBarrier(envTexture, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_NONE, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL).MipLevels(0, 1);
			PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, {postDispatchBarrier});

			ExecuteImmediateCommandBuffer(commandBuffer);

			vkDestroyPipeline(mDevice->GetHandle(), compPipeline, nullptr);
			DestroyTexture(envTextureEquirect);

			GenerateMipmaps(envTexture);
		}

		// prefilter map
		{
			const uint32_t numMipTailLevels = kEnvMapLevels - 1;

			VkPipeline compPipeline;
			{
				const VkSpecializationMapEntry specializationMap = {0, 0, sizeof(uint32_t)};
				const uint32_t specializationData[] = {numMipTailLevels};

				const VkSpecializationInfo specializationInfo = {1, &specializationMap, sizeof(specializationData), specializationData};

				std::vector<uint32_t> specularmap_comp;
				VK_CHECK(GlslToSpv(VK_SHADER_STAGE_COMPUTE_BIT, ReadFile(std::string(ASSETS_DIR) + "shaders/specularmap.comp"), specularmap_comp));

				compPipeline = CreateComputePipeline(specularmap_comp, computePipelineLayout, &specializationInfo);
			}

			// Copy base mipmap level into destination environment map.
			{
				VkCommandBuffer commandBuffer = BeginImmediateCommandBuffer();

				const std::vector<ImageMemoryBarrier> preCopyBarriers = {
					ImageMemoryBarrier(envTexture, VK_ACCESS_NONE, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL).MipLevels(0, 1),
					ImageMemoryBarrier(mEnvTexture, VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL),
				};

				const std::vector<ImageMemoryBarrier> postCopyBarriers = {
					ImageMemoryBarrier(envTexture, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL).MipLevels(0, 1),
					ImageMemoryBarrier(mEnvTexture, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL),
				};

				PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, preCopyBarriers);

				VkImageCopy copyRegion{};
				copyRegion.extent = {mEnvTexture.width, mEnvTexture.height, 1};
				copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.srcSubresource.layerCount = mEnvTexture.layers;
				copyRegion.dstSubresource = copyRegion.srcSubresource;

				vkCmdCopyImage(commandBuffer,
							   envTexture.image.handle,
							   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							   mEnvTexture.image.handle,
							   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							   1,
							   &copyRegion);

				PipelineBarrier(commandBuffer,
								VK_PIPELINE_STAGE_TRANSFER_BIT,
								VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
								postCopyBarriers);

				// other mip level prefilter
				std::vector<VkImageView> envTextureMipTailViews;
				std::vector<VkDescriptorImageInfo> envTextureMipTailDescriptors;
				const VkDescriptorImageInfo inputTexture = {VK_NULL_HANDLE, envTexture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
				UpdateDescriptorSet(computeDescriptorSet, BINDING_INPUT_TEXTURE, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, {inputTexture});

				for (uint32_t level = 1; level < kEnvMapLevels; ++level)
				{
					envTextureMipTailViews.emplace_back(CreateTextureView(mEnvTexture, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, level, 1));
					envTextureMipTailDescriptors.emplace_back(VkDescriptorImageInfo{VK_NULL_HANDLE, envTextureMipTailViews[level - 1], VK_IMAGE_LAYOUT_GENERAL});
				}

				UpdateDescriptorSet(computeDescriptorSet, BINDING_OUTPUT_MIP_TAIL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, envTextureMipTailDescriptors);

				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compPipeline);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeDescriptorSet, 0, nullptr);

				const float deltaRoughness = 1.0f / Math::Max(float(numMipTailLevels), 1.0f);
				for (uint32_t level = 1, size = kEnvMapSize / 2; level < kEnvMapLevels; ++level, size /= 2)
				{
					const uint32_t numGroups = Math::Max((uint32_t)1, size / 32);

					const SpecularFilterPushConstants pushConstants = {level - 1, level * deltaRoughness};
					vkCmdPushConstants(commandBuffer, computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SpecularFilterPushConstants), &pushConstants);
					vkCmdDispatch(commandBuffer, numGroups, numGroups, 6);
				}

				const auto barrier = ImageMemoryBarrier(mEnvTexture, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_NONE, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, {barrier});

				ExecuteImmediateCommandBuffer(commandBuffer);

				for (const auto &mipTailView : envTextureMipTailViews)
					vkDestroyImageView(mDevice->GetHandle(), mipTailView, nullptr);
				vkDestroyPipeline(mDevice->GetHandle(), compPipeline, nullptr);
				DestroyTexture(envTexture);
			}
		}

		// diffuse irradiance map
		{
			std::vector<uint32_t> irradiance_comp;
			VK_CHECK(GlslToSpv(VK_SHADER_STAGE_COMPUTE_BIT, ReadFile(std::string(ASSETS_DIR) + "shaders/irradiancemap.comp"), irradiance_comp));

			VkPipeline compPipeline = CreateComputePipeline(irradiance_comp, computePipelineLayout);

			const VkDescriptorImageInfo inputTexture = {VK_NULL_HANDLE, mEnvTexture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
			const VkDescriptorImageInfo outputTexture = {VK_NULL_HANDLE, mIrradianceTexture.view, VK_IMAGE_LAYOUT_GENERAL};
			UpdateDescriptorSet(computeDescriptorSet, BINDING_INPUT_TEXTURE, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, {inputTexture});
			UpdateDescriptorSet(computeDescriptorSet, BINDING_OUTPUT_TEXTURE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, {outputTexture});

			VkCommandBuffer commandBuffer = BeginImmediateCommandBuffer();

			const auto preDispatchBarrier = ImageMemoryBarrier(mIrradianceTexture, VK_ACCESS_NONE, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
			PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, {preDispatchBarrier});

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compPipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeDescriptorSet, 0, nullptr);
			vkCmdDispatch(commandBuffer, kIrradianceMapSize / 32, kIrradianceMapSize / 32, 6);

			const auto postDispatcher = ImageMemoryBarrier(mIrradianceTexture, VK_ACCESS_SHADER_WRITE_BIT, 0, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, {postDispatcher});

			ExecuteImmediateCommandBuffer(commandBuffer);
			vkDestroyPipeline(mDevice->GetHandle(), compPipeline, nullptr);
		}

		// brdf map
		{
			std::vector<uint32_t> brdf_comp;
			VK_CHECK(GlslToSpv(VK_SHADER_STAGE_COMPUTE_BIT, ReadFile(std::string(ASSETS_DIR) + "shaders/pbr_brdf.comp"), brdf_comp));

			VkPipeline compPipeline = CreateComputePipeline(brdf_comp, computePipelineLayout);
			const VkDescriptorImageInfo outputTexture = {VK_NULL_HANDLE, mBrdfLut.view, VK_IMAGE_LAYOUT_GENERAL};
			UpdateDescriptorSet(computeDescriptorSet, BINDING_OUTPUT_TEXTURE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, {outputTexture});

			VkCommandBuffer commandBuffer = BeginImmediateCommandBuffer();

			const auto preDispatchBarrier = ImageMemoryBarrier(mBrdfLut, VK_ACCESS_NONE, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
			PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, {preDispatchBarrier});

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compPipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeDescriptorSet, 0, nullptr);
			vkCmdDispatch(commandBuffer, kBRDF_LUT_Size / 32, kBRDF_LUT_Size / 32, 6);

			const auto postDispatchBarrier = ImageMemoryBarrier(mBrdfLut, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_NONE, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, {postDispatchBarrier});

			ExecuteImmediateCommandBuffer(commandBuffer);
			vkDestroyPipeline(mDevice->GetHandle(), compPipeline, nullptr);
		}
	}

	vkDestroyDescriptorSetLayout(mDevice->GetHandle(), setLayout.uniforms, nullptr);
	vkDestroyDescriptorSetLayout(mDevice->GetHandle(), setLayout.pbr, nullptr);
	vkDestroyDescriptorSetLayout(mDevice->GetHandle(), setLayout.skybox, nullptr);
	vkDestroyDescriptorSetLayout(mDevice->GetHandle(), setLayout.tonemap, nullptr);
	vkDestroyDescriptorSetLayout(mDevice->GetHandle(), setLayout.compute, nullptr);

	vkDestroySampler(mDevice->GetHandle(), computeSampler, nullptr);
	vkDestroyPipelineLayout(mDevice->GetHandle(), computePipelineLayout, nullptr);
	vkDestroyDescriptorPool(mDevice->GetHandle(), computeDescriptorPool, nullptr);
}

Renderer::Renderer()
{
	
}

Renderer::~Renderer()
{
	mDevice->WaitIdle();

	DestroyTexture(mEnvTexture);
	DestroyTexture(mIrradianceTexture);
	DestroyTexture(mBrdfLut);

	DestroyModelBuffer(mSkyboxModelBuffer);
	DestroyModelBuffer(mPbrModelBuffer);

	DestroyTexture(mAlbedoTexture);
	DestroyTexture(mNormalTexture);
	DestroyTexture(mMetalnessTexture);
	DestroyTexture(mRoughnessTexture);

	DestroyUniformBuffer(mUniformBuffer);

	vkDestroySampler(mDevice->GetHandle(), mDefaultSampler, nullptr);
	vkDestroySampler(mDevice->GetHandle(), mSpecularBRDFSampler, nullptr);

	vkDestroyPipelineLayout(mDevice->GetHandle(), mPbrPipelineLayout, nullptr);
	vkDestroyPipeline(mDevice->GetHandle(), mPbrPipeline, nullptr);
	vkDestroyPipelineLayout(mDevice->GetHandle(), mSkyboxPipelineLayout, nullptr);
	vkDestroyPipeline(mDevice->GetHandle(), mSkyboxPipeline, nullptr);
	vkDestroyPipelineLayout(mDevice->GetHandle(), mToneMapPipelineLayout, nullptr);
	vkDestroyPipeline(mDevice->GetHandle(), mToneMapPipeline, nullptr);

	vkDestroyRenderPass(mDevice->GetHandle(), mRenderPass, nullptr);

	for (uint32_t i = 0; i < mNumFrames; ++i)
	{
		DestroyRenderTarget(mRenderTargets[i]);
		if (mRenderSamples > 1)
			DestroyRenderTarget(mResolveRenderTargets[i]);

		vkDestroyFramebuffer(mDevice->GetHandle(), mFramebuffers[i], nullptr);
		vkDestroyFence(mDevice->GetHandle(), mSubmitFences[i], nullptr);
	}

	vkDestroyDescriptorPool(mDevice->GetHandle(), mDescriptorPool, nullptr);
	vkDestroyCommandPool(mDevice->GetHandle(), mCommandPool, nullptr);
	vkDestroyFence(mDevice->GetHandle(), mPresentationFence, nullptr);
}
void Renderer::Render(const PbrScene &scene)
{
	const VkDeviceSize zeroOffset = 0;

	Matrix4f projectionMatrix = scene.mCamera.mProjectionMatrix;
	Matrix4f skyboxRotationMatrix = scene.mSkyboxRotationMatrix;
	Matrix4f viewMatrix = scene.mCamera.mViewMatrix;

	Vector3f eyePosition = scene.mCamera.mPosition;

	VkCommandBuffer commandBuffer = mCommandBuffers[mFrameIndex];
	VkImage swapChainImage = mSwapChain->GetImages()[mFrameIndex];
	VkFramebuffer framebuffer = mFramebuffers[mFrameIndex];

	VkDescriptorSet uniformDescriptorSet = mUniformsDescriptorSets[mFrameIndex];
	VkDescriptorSet toneMapDexcriptorSet = mToneMapDescriptorSets[mFrameIndex];

	// update uniform
	{
		TransformUniforms *const transformUniforms = mTransformUniforms[mFrameIndex].as<TransformUniforms>();
		transformUniforms->projectionMatrix = projectionMatrix;
		transformUniforms->viewMatrix = viewMatrix;
		transformUniforms->skyboxRotationMatrix = skyboxRotationMatrix;

		ShadingUniforms *const shadingUniforms = mShadingUniforms[mFrameIndex].as<ShadingUniforms>();
		shadingUniforms->eyePosition = Vector4f(eyePosition, 0.0f);
		for (int32_t i = 0; i < LIGHT_NUM; ++i)
			shadingUniforms->lights[i] = scene.mLights[i];
	}

	// begin recommand cur frame command buffer
	{
		VkCommandBufferBeginInfo commandBufferBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkResetCommandBuffer(commandBuffer, 0);
		vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
	}
	// begin render pass

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = {0.2f, 0.3f, 0.5f, 1.0f};
	clearValues[1].depthStencil.depth = 1.0f;

	VkRenderPassBeginInfo renderPassBeginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
	renderPassBeginInfo.renderPass = mRenderPass;
	renderPassBeginInfo.framebuffer = framebuffer;
	renderPassBeginInfo.renderArea = mFrameRect;
	renderPassBeginInfo.clearValueCount = (uint32_t)clearValues.size();
	renderPassBeginInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	// draw skybox
	{
		const std::array<VkDescriptorSet, 2> descriptorSets = {
			uniformDescriptorSet,
			mSkyboxDescriptorSet};

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mSkyboxPipeline);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mSkyboxPipelineLayout, 0, (uint32_t)descriptorSets.size(), descriptorSets.data(), 0, nullptr);

		for (const auto &meshBuf : mSkyboxModelBuffer)
		{
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &meshBuf.vertexBuffer.handle, &zeroOffset);
			vkCmdBindIndexBuffer(commandBuffer, meshBuf.indexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(commandBuffer, meshBuf.numElements, 1, 0, 0, 0);
		}
	}

	// draw PBR model
	{
		const std::array<VkDescriptorSet, 1> descriptorSets = {
			mPbrDescriptorSet,
		};

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPbrPipeline);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPbrPipelineLayout, 1, descriptorSets.size(), descriptorSets.data(), 0, nullptr);

		for (const auto &meshBuf : mPbrModelBuffer)
		{
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &meshBuf.vertexBuffer.handle, &zeroOffset);
			vkCmdBindIndexBuffer(commandBuffer, meshBuf.indexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(commandBuffer, meshBuf.numElements, 1, 0, 0, 0);
		}
	}

	vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

	// post processing
	{
		const std::array<VkDescriptorSet, 1> descriptorSets = {
			toneMapDexcriptorSet};

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mToneMapPipeline);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mToneMapPipelineLayout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	}

	vkCmdEndRenderPass(commandBuffer);
	vkEndCommandBuffer(commandBuffer);

	// submit

	VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(mDevice->GetGraphicsQueue()->GetHandle(), 1, &submitInfo, mSubmitFences[mFrameIndex]);

	PresentFrame();
}
Resource<VkBuffer> Renderer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags) const
{
	Resource<VkBuffer> buffer;

	VkBufferCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	createInfo.size = size;
	createInfo.usage = usage;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK(vkCreateBuffer(mDevice->GetHandle(), &createInfo, nullptr, &buffer.handle));

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(mDevice->GetHandle(), buffer.handle, &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = ChooseMemoryType(memoryRequirements, memoryFlags);

	VK_CHECK(vkAllocateMemory(mDevice->GetHandle(), &allocateInfo, nullptr, &buffer.memory));
	VK_CHECK(vkBindBufferMemory(mDevice->GetHandle(), buffer.handle, buffer.memory, 0));

	buffer.allocateSize = allocateInfo.allocationSize;
	buffer.memoryTypeIndex = allocateInfo.memoryTypeIndex;

	return buffer;
}
Resource<VkImage> Renderer::CreateImage(uint32_t width, uint32_t height, uint32_t layers, uint32_t levels, VkFormat format, uint32_t samples, VkImageUsageFlags usage) const
{
	assert(width > 0);
	assert(height > 0);
	assert(levels > 0);
	assert(layers == 1 || layers == 6);
	assert(samples > 0 && samples <= 64);

	Resource<VkImage> image;

	VkImageCreateInfo createInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	createInfo.flags = (layers == 6) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.format = format;
	createInfo.extent = {width, height, 1};
	createInfo.mipLevels = levels;
	createInfo.arrayLayers = layers;
	createInfo.samples = static_cast<VkSampleCountFlagBits>(samples);
	createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	createInfo.usage = usage;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VK_CHECK(vkCreateImage(mDevice->GetHandle(), &createInfo, nullptr, &image.handle));

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(mDevice->GetHandle(), image.handle, &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = ChooseMemoryType(memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VK_CHECK(vkAllocateMemory(mDevice->GetHandle(), &allocateInfo, nullptr, &image.memory));
	VK_CHECK(vkBindImageMemory(mDevice->GetHandle(), image.handle, image.memory, 0));

	image.allocateSize = allocateInfo.allocationSize;
	image.memoryTypeIndex = allocateInfo.memoryTypeIndex;

	return image;
}
void Renderer::DestroyBuffer(Resource<VkBuffer> &buffer) const
{
	if (buffer.handle)
		vkDestroyBuffer(mDevice->GetHandle(), buffer.handle, nullptr);
	if (buffer.memory)
		vkFreeMemory(mDevice->GetHandle(), buffer.memory, nullptr);
	buffer = {};
}
void Renderer::DestroyImage(Resource<VkImage> &image) const
{
	if (image.handle)
		vkDestroyImage(mDevice->GetHandle(), image.handle, nullptr);
	if (image.memory)
		vkFreeMemory(mDevice->GetHandle(), image.memory, nullptr);
	image = {};
}
ModelBuffer Renderer::CreateModelBuffer(const _Model &model) const
{
	ModelBuffer result;

	for (auto meshInstacne : model.GetMeshInstances())
		result.emplace_back(CreateMeshBuffer(meshInstacne.mesh));

	return result;
}
void Renderer::DestroyModelBuffer(ModelBuffer &model) const
{
	for (auto &meshBuffer : model)
		DestroyMeshBuffer(meshBuffer);
}
MeshBuffer Renderer::CreateMeshBuffer(const _Mesh &mesh) const
{
	MeshBuffer buffer;

	buffer.numElements = mesh.mIndices.size();

	const size_t vertexDataSize = mesh.mVertices.size() * sizeof(_Vertex);
	const size_t indexDataSize = mesh.mIndices.size() * sizeof(uint32_t);

	buffer.vertexBuffer = CreateBuffer(vertexDataSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	buffer.indexBuffer = CreateBuffer(indexDataSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	bool usingStagingForVertexBuffer = false;
	bool usingStagingForIndexBuffer = false;

	auto stagingVertexBuffer = buffer.vertexBuffer;
	if (MemoryTypeNeedsStaging(buffer.vertexBuffer.memoryTypeIndex))
	{
		stagingVertexBuffer = CreateBuffer(buffer.vertexBuffer.allocateSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		usingStagingForVertexBuffer = true;
	}

	auto stagingIndexBuffer = buffer.indexBuffer;
	if (MemoryTypeNeedsStaging(buffer.indexBuffer.memoryTypeIndex))
	{
		stagingIndexBuffer = CreateBuffer(buffer.indexBuffer.allocateSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		usingStagingForIndexBuffer = true;
	}

	CopyToDevice(stagingVertexBuffer.memory, mesh.mVertices.data(), vertexDataSize);
	CopyToDevice(stagingIndexBuffer.memory, mesh.mIndices.data(), indexDataSize);

	if (usingStagingForVertexBuffer || usingStagingForIndexBuffer)
	{
		VkCommandBuffer commandBuffer = BeginImmediateCommandBuffer();

		if (usingStagingForVertexBuffer)
		{
			const VkBufferCopy bufferCopyRegion = {0, 0, vertexDataSize};
			vkCmdCopyBuffer(commandBuffer, stagingVertexBuffer.handle, buffer.vertexBuffer.handle, 1, &bufferCopyRegion);
		}

		if (usingStagingForIndexBuffer)
		{
			const VkBufferCopy bufferCopyRegion = {0, 0, indexDataSize};
			vkCmdCopyBuffer(commandBuffer, stagingIndexBuffer.handle, buffer.indexBuffer.handle, 1, &bufferCopyRegion);
		}

		ExecuteImmediateCommandBuffer(commandBuffer);
	}

	if (usingStagingForVertexBuffer)
		DestroyBuffer(stagingVertexBuffer);

	if (usingStagingForIndexBuffer)
		DestroyBuffer(stagingIndexBuffer);

	return buffer;
}
void Renderer::DestroyMeshBuffer(MeshBuffer &meshBuffer) const
{
	DestroyBuffer(meshBuffer.vertexBuffer);
	DestroyBuffer(meshBuffer.indexBuffer);
	meshBuffer = {};
}
Texture Renderer::CreateTexture(uint32_t width, uint32_t height, uint32_t layers, VkFormat format, uint32_t levels, VkImageUsageFlags additionUsage) const
{
	assert(width > 0 && height > 0);
	assert(layers > 0);

	Texture texture;
	texture.width = width;
	texture.height = height;
	texture.layers = layers;
	texture.levels = (levels > 0) ? levels : Math::NumMipmapLevels(width, height);

	VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | additionUsage;
	if (texture.levels > 1)
		usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT; // For mipmap generation

	texture.image = CreateImage(width, height, layers, texture.levels, format, 1, usage);
	texture.view = CreateTextureView(texture, format, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);

	return texture;
}
Texture Renderer::CreateTexture(const Image &image, VkFormat format, uint32_t levels) const
{
	Texture texture = CreateTexture(image.mWidth, image.mHeight, 1, format, levels);

	const size_t pixelDataSize = image.mWidth * image.mHeight * image.BytesPerPixel();

	Resource<VkBuffer> stagingBuffer = CreateBuffer(pixelDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	CopyToDevice(stagingBuffer.memory, image.pixels<void>(), pixelDataSize);

	VkCommandBuffer commandBuffer = BeginImmediateCommandBuffer();

	{
		const auto barrier = ImageMemoryBarrier(texture, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL).MipLevels(0, 1);
		PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, {barrier});
	}

	VkBufferImageCopy copyRegion{};
	copyRegion.bufferOffset = 0;
	copyRegion.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
	copyRegion.imageExtent = {texture.width, texture.height, 1};
	vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.handle, texture.image.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

	{
		// If we're going to generate mipmaps transition base mip to transfer src layout, otherwise use shader read only layout.
		const VkImageLayout finalBaseMipLayout = (texture.levels > 1) ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		const auto barrier = ImageMemoryBarrier(texture, VK_ACCESS_TRANSFER_WRITE_BIT, 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, finalBaseMipLayout).MipLevels(0, 1);
		PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, {barrier});
	}

	ExecuteImmediateCommandBuffer(commandBuffer);

	DestroyBuffer(stagingBuffer);

	if (texture.levels > 1)
		GenerateMipmaps(texture);
	return texture;
}
VkImageView Renderer::CreateTextureView(const Texture &texture, VkFormat format, VkImageAspectFlags aspectMask, uint32_t baseMipLevel, uint32_t numMipLevels) const
{
	VkImageViewCreateInfo viewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
	viewCreateInfo.image = texture.image.handle;
	viewCreateInfo.viewType = (texture.layers == 6) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.format = format;
	viewCreateInfo.subresourceRange.aspectMask = aspectMask;
	viewCreateInfo.subresourceRange.baseMipLevel = baseMipLevel;
	viewCreateInfo.subresourceRange.levelCount = numMipLevels;
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;
	viewCreateInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

	VkImageView view;
	VK_CHECK(vkCreateImageView(mDevice->GetHandle(), &viewCreateInfo, nullptr, &view));

	return view;
}
void Renderer::GenerateMipmaps(const Texture &texture) const
{
	assert(texture.levels > 1);

	VkCommandBuffer commandBuffer = BeginImmediateCommandBuffer();

	// Iterate through mip chain and consecutively blit from previous level to next level with linear filtering.
	for (uint32_t level = 1, prevLevelWidth = texture.width, prevLevelHeight = texture.height; level < texture.levels; ++level, prevLevelWidth /= 2, prevLevelHeight /= 2)
	{
		const auto preBlitBarrier = ImageMemoryBarrier(texture, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL).MipLevels(level, 1);
		PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, {preBlitBarrier});

		VkImageBlit region = {};
		region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level - 1, 0, texture.layers};
		region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level, 0, texture.layers};
		region.srcOffsets[1] = {int32_t(prevLevelWidth), int32_t(prevLevelHeight), 1};
		region.dstOffsets[1] = {int32_t(prevLevelWidth) / 2, int32_t(prevLevelHeight) / 2, 1};
		vkCmdBlitImage(commandBuffer,
					   texture.image.handle,
					   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					   texture.image.handle,
					   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					   1,
					   &region,
					   VK_FILTER_LINEAR);
		const auto postBlitBarrier = ImageMemoryBarrier(texture, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL).MipLevels(level, 1);
		PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, {postBlitBarrier});
	}

	// Transition whole mip chain to shader read only layout.
	{
		const auto barrier = ImageMemoryBarrier(texture, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_NONE, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, {barrier});
	}

	ExecuteImmediateCommandBuffer(commandBuffer);
}
void Renderer::DestroyTexture(Texture &texture) const
{
	if (texture.view)
		vkDestroyImageView(mDevice->GetHandle(), texture.view, nullptr);
	DestroyImage(texture.image);
}
RenderTarget Renderer::CreateRenderTarget(uint32_t width, uint32_t height, uint32_t samples, VkFormat colorFormat, VkFormat depthFormat) const
{
	assert(samples > 0 && samples <= 64);

	RenderTarget target = {};
	target.width = width;
	target.height = height;
	target.samples = samples;
	target.colorFormat = colorFormat;
	target.depthFormat = depthFormat;

	VkImageUsageFlags colorImageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	if (samples == 1)
		colorImageUsage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

	if (colorFormat != VK_FORMAT_UNDEFINED)
		target.colorImage = CreateImage(width, height, 1, 1, colorFormat, samples, colorImageUsage);

	if (depthFormat != VK_FORMAT_UNDEFINED)
		target.depthImage = CreateImage(width, height, 1, 1, depthFormat, samples, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

	VkImageViewCreateInfo viewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.subresourceRange.levelCount = 1;
	viewCreateInfo.subresourceRange.layerCount = 1;

	if (target.colorImage.handle)
	{
		viewCreateInfo.image = target.colorImage.handle;
		viewCreateInfo.format = colorFormat;
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		VK_CHECK(vkCreateImageView(mDevice->GetHandle(), &viewCreateInfo, nullptr, &target.colorView));
	}

	if (target.depthImage.handle)
	{
		viewCreateInfo.image = target.depthImage.handle;
		viewCreateInfo.format = depthFormat;
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		VK_CHECK(vkCreateImageView(mDevice->GetHandle(), &viewCreateInfo, nullptr, &target.depthView));
	}

	return target;
}
void Renderer::DestroyRenderTarget(RenderTarget &rt) const
{
	DestroyImage(rt.colorImage);
	DestroyImage(rt.depthImage);

	if (rt.colorView)
		vkDestroyImageView(mDevice->GetHandle(), rt.colorView, nullptr);

	if (rt.depthView)
		vkDestroyImageView(mDevice->GetHandle(), rt.depthView, nullptr);

	rt = {};
}
UniformBufferWrap Renderer::CreateUniformBuffer(VkDeviceSize capacity) const
{
	assert(capacity > 0);

	UniformBufferWrap ubo = {};
	ubo.buffer = CreateBuffer(capacity, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	ubo.capacity = capacity;

	VK_CHECK(vkMapMemory(mDevice->GetHandle(), ubo.buffer.memory, 0, VK_WHOLE_SIZE, 0, &ubo.hostMemoryPtr));

	return ubo;
}
void Renderer::DestroyUniformBuffer(UniformBufferWrap &uniformBuffer) const
{
	if (uniformBuffer.hostMemoryPtr && uniformBuffer.buffer.memory)
		vkUnmapMemory(mDevice->GetHandle(), uniformBuffer.buffer.memory);
	DestroyBuffer(uniformBuffer.buffer);
	uniformBuffer = {};
}
UniformBufferWrapAllocation Renderer::AllocFromUniformBuffer(UniformBufferWrap &buffer, VkDeviceSize size) const
{
	const VkDeviceSize minAlignment = mDevice->GetPhysicalProps().limits.minUniformBufferOffsetAlignment;
	const VkDeviceSize alignedSize = Math::RoundToPowerOfTwo(size, minAlignment);
	if (alignedSize > mDevice->GetPhysicalProps().limits.maxUniformBufferRange)
		throw std::invalid_argument("Requested uniform buffer sub-allocation size exceeds maxUniformBufferRange of current physical device");

	if (buffer.cursor + alignedSize > buffer.capacity)
		throw std::overflow_error("Out of uniform buffer capacity while allocating memory");

	UniformBufferWrapAllocation allocation;
	allocation.descriptorInfo.buffer = buffer.buffer.handle;
	allocation.descriptorInfo.offset = buffer.cursor;
	allocation.descriptorInfo.range = alignedSize;
	allocation.hostMemoryPtr = reinterpret_cast<uint8_t *>(buffer.hostMemoryPtr) + buffer.cursor;

	buffer.cursor += alignedSize;
	return allocation;
}
VkDescriptorSet Renderer::AllocateDescriptorSet(VkDescriptorPool pool, VkDescriptorSetLayout layout) const
{
	VkDescriptorSet descriptorSet;
	VkDescriptorSetAllocateInfo allocateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
	allocateInfo.descriptorPool = pool;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &layout;
	vkAllocateDescriptorSets(mDevice->GetHandle(), &allocateInfo, &descriptorSet);
	return descriptorSet;
}
void Renderer::UpdateDescriptorSet(VkDescriptorSet dstSet, uint32_t dstBinding, VkDescriptorType descriptorType, const std::vector<VkDescriptorImageInfo> &descriptors) const
{
	VkWriteDescriptorSet writeDescriptorSet = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
	writeDescriptorSet.dstSet = dstSet;
	writeDescriptorSet.dstBinding = dstBinding;
	writeDescriptorSet.descriptorType = descriptorType;
	writeDescriptorSet.descriptorCount = descriptors.size();
	writeDescriptorSet.pImageInfo = descriptors.data();

	vkUpdateDescriptorSets(mDevice->GetHandle(), 1, &writeDescriptorSet, 0, nullptr);
}

void Renderer::UpdateDescriptorSet(VkDescriptorSet dstSet, uint32_t dstBinding, VkDescriptorType descriptorType, const std::vector<VkDescriptorBufferInfo> &descriptors) const
{
	VkWriteDescriptorSet writeDescriptorSet = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
	writeDescriptorSet.dstSet = dstSet;
	writeDescriptorSet.dstBinding = dstBinding;
	writeDescriptorSet.descriptorType = descriptorType;
	writeDescriptorSet.descriptorCount = descriptors.size();
	writeDescriptorSet.pBufferInfo = descriptors.data();

	vkUpdateDescriptorSets(mDevice->GetHandle(), 1, &writeDescriptorSet, 0, nullptr);
}

VkDescriptorSetLayout Renderer::CreateDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding> *bindings) const
{
	VkDescriptorSetLayout layout;
	VkDescriptorSetLayoutCreateInfo createInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
	if (bindings && bindings->size() > 0)
	{
		createInfo.bindingCount = bindings->size();
		createInfo.pBindings = bindings->data();
	}

	VK_CHECK(vkCreateDescriptorSetLayout(mDevice->GetHandle(), &createInfo, nullptr, &layout));
	return layout;
}
VkPipelineLayout Renderer::CreatePipelineLayout(const std::vector<VkDescriptorSetLayout> *setLayouts, const std::vector<VkPushConstantRange> *pushConstants) const
{
	VkPipelineLayout layout;
	VkPipelineLayoutCreateInfo createInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

	if (setLayouts && setLayouts->size() > 0)
	{
		createInfo.setLayoutCount = setLayouts->size();
		createInfo.pSetLayouts = setLayouts->data();
	}

	if (pushConstants && pushConstants->size() > 0)
	{
		createInfo.pushConstantRangeCount = pushConstants->size();
		createInfo.pPushConstantRanges = pushConstants->data();
	}

	VK_CHECK(vkCreatePipelineLayout(mDevice->GetHandle(), &createInfo, nullptr, &layout));

	return layout;
}
VkPipeline Renderer::CreateGraphicsPipeline(uint32_t subpass, const std::vector<uint32_t> &vs, const std::vector<uint32_t> &fs, VkPipelineLayout layout, const std::vector<VkVertexInputBindingDescription> *vertexInputBindings, const std::vector<VkVertexInputAttributeDescription> *vertexAttributes, const VkPipelineMultisampleStateCreateInfo *multisampleState, const VkPipelineDepthStencilStateCreateInfo *depthStencilState) const
{
	const VkViewport defaultViewport = {
		0.0f,
		0.0f,
		(float)mFrameRect.extent.width,
		(float)mFrameRect.extent.height,
		0.0f,
		1.0f,
	};

	const VkPipelineMultisampleStateCreateInfo defaultMultiSampleState = {
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_SAMPLE_COUNT_1_BIT,
	};

	VkPipelineColorBlendAttachmentState defaultColorBlendAttachmentState = {};
	defaultColorBlendAttachmentState.blendEnable = VK_FALSE;
	defaultColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkShaderModule vertexShader = CreateShaderModuleFromSPV(vs);
	VkShaderModule fragmentShader = CreateShaderModuleFromSPV(fs);

	const std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
		{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vertexShader, "main", nullptr},
		{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader, "main", nullptr},
	};

	VkPipelineVertexInputStateCreateInfo vertexInputState = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
	if (vertexInputBindings)
	{
		vertexInputState.vertexBindingDescriptionCount = vertexInputBindings->size();
		vertexInputState.pVertexBindingDescriptions = vertexInputBindings->data();
	}

	if (vertexAttributes)
	{
		vertexInputState.vertexAttributeDescriptionCount = vertexAttributes->size();
		vertexInputState.pVertexAttributeDescriptions = vertexAttributes->data();
	}

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyState.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	viewportState.pViewports = &defaultViewport;
	viewportState.pScissors = &mFrameRect;

	VkPipelineRasterizationStateCreateInfo rasterizationState = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationState.cullMode = VK_CULL_MODE_NONE;
	rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationState.lineWidth = 1.0f;

	const VkPipelineColorBlendAttachmentState colorBlendAttachmentStates[] = {
		defaultColorBlendAttachmentState};

	VkPipelineColorBlendStateCreateInfo colorBlendState = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = colorBlendAttachmentStates;

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
	pipelineCreateInfo.stageCount = shaderStages.size();
	pipelineCreateInfo.pStages = shaderStages.data();
	pipelineCreateInfo.pVertexInputState = &vertexInputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pMultisampleState = multisampleState ? multisampleState : &defaultMultiSampleState;
	pipelineCreateInfo.pDepthStencilState = depthStencilState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.layout = layout;
	pipelineCreateInfo.renderPass = mRenderPass;
	pipelineCreateInfo.subpass = subpass;

	VkPipeline pipeline;
	VK_CHECK(vkCreateGraphicsPipelines(mDevice->GetHandle(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline));

	vkDestroyShaderModule(mDevice->GetHandle(), vertexShader, nullptr);
	vkDestroyShaderModule(mDevice->GetHandle(), fragmentShader, nullptr);

	return pipeline;
}
VkPipeline Renderer::CreateComputePipeline(const std::vector<uint32_t> &cs, VkPipelineLayout layout, const VkSpecializationInfo *specializationInfo) const
{
	VkShaderModule computeShader = CreateShaderModuleFromSPV(cs);

	const VkPipelineShaderStageCreateInfo shaderStage = {
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		nullptr,
		0,
		VK_SHADER_STAGE_COMPUTE_BIT,
		computeShader,
		"main",
		specializationInfo};

	VkComputePipelineCreateInfo createInfo = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
	createInfo.stage = shaderStage;
	createInfo.layout = layout;

	VkPipeline pipeline;
	VK_CHECK(vkCreateComputePipelines(mDevice->GetHandle(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline));

	vkDestroyShaderModule(mDevice->GetHandle(), computeShader, nullptr);

	return pipeline;
}
VkShaderModule Renderer::CreateShaderModuleFromSPV(const std::vector<uint32_t> &spvCode) const
{
	VkShaderModuleCreateInfo createInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.codeSize = spvCode.size() * sizeof(uint32_t);
	createInfo.pCode = spvCode.data();

	VkShaderModule shaderModule = VK_NULL_HANDLE;
	VK_CHECK(vkCreateShaderModule(mDevice->GetHandle(), &createInfo, nullptr, &shaderModule));

	return shaderModule;
}
VkCommandBuffer Renderer::BeginImmediateCommandBuffer() const
{
	VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VK_CHECK(vkBeginCommandBuffer(mCommandBuffers[mFrameIndex], &beginInfo))
	return mCommandBuffers[mFrameIndex];
}
void Renderer::ExecuteImmediateCommandBuffer(VkCommandBuffer commandBuffer) const
{
	VK_CHECK(vkEndCommandBuffer(commandBuffer))

	VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(mDevice->GetGraphicsQueue()->GetHandle(), 1, &submitInfo, VK_NULL_HANDLE);
	mDevice->GetGraphicsQueue()->WaitIdle();

	VK_CHECK(vkResetCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
}
void Renderer::CopyToDevice(VkDeviceMemory deviceMemory, const void *data, size_t size) const
{
	const VkMappedMemoryRange flushRange = {
		VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		nullptr,
		deviceMemory,
		0,
		VK_WHOLE_SIZE};

	void *mappedMemory;
	VK_CHECK(vkMapMemory(mDevice->GetHandle(), deviceMemory, 0, VK_WHOLE_SIZE, 0, &mappedMemory));

	std::memcpy(mappedMemory, data, size);
	vkFlushMappedMemoryRanges(mDevice->GetHandle(), 1, &flushRange);
	vkUnmapMemory(mDevice->GetHandle(), deviceMemory);
}
void Renderer::PipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const std::vector<ImageMemoryBarrier> &barriers) const
{
	vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, (uint32_t)barriers.size(), reinterpret_cast<const VkImageMemoryBarrier *>(barriers.data()));
}
void Renderer::PresentFrame()
{
	VkResult presentResult;

	VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &mSwapChain->GetHandle();
	presentInfo.pImageIndices = &mFrameIndex;
	presentInfo.pResults = &presentResult;

	VK_CHECK(vkQueuePresentKHR(mDevice->GetGraphicsQueue()->GetHandle(), &presentInfo));
	VK_CHECK(presentResult);

	VK_CHECK(vkAcquireNextImageKHR(mDevice->GetHandle(), mSwapChain->GetHandle(), UINT64_MAX, VK_NULL_HANDLE, mPresentationFence, &mFrameIndex));

	const VkFence fences[] = {
		mPresentationFence,
		mSubmitFences[mFrameIndex],
	};

	const uint32_t numFencesToWaitFor = (mFrameCount < mFrameIndex) ? 1 : 2;
	vkWaitForFences(mDevice->GetHandle(), numFencesToWaitFor, fences, VK_TRUE, UINT64_MAX);
	vkResetFences(mDevice->GetHandle(), numFencesToWaitFor, fences);

	++mFrameCount;
}

uint32_t Renderer::QueryRenderTargetFormatMaxSamples(VkFormat format, VkImageUsageFlags usage) const
{
	VkImageFormatProperties properties;
	VK_CHECK(vkGetPhysicalDeviceImageFormatProperties(mDevice->GetPhysicalHandle(), format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &properties))

	for (VkSampleCountFlags maxSampleCount = VK_SAMPLE_COUNT_64_BIT; maxSampleCount > VK_SAMPLE_COUNT_1_BIT; maxSampleCount >>= 1)
		if (properties.sampleCounts & maxSampleCount)
			return static_cast<uint32_t>(maxSampleCount);

	return 1;
}
uint32_t Renderer::ChooseMemoryType(const VkMemoryRequirements &memoryRequirements, VkMemoryPropertyFlags perferredFlags, VkMemoryPropertyFlags requiredFlags) const
{
	if (requiredFlags == 0)
		requiredFlags = perferredFlags;

	uint32_t memoryType = mDevice->FindMemoryType(memoryRequirements.memoryTypeBits,perferredFlags);

	if (memoryType == -1 && requiredFlags != perferredFlags)
		memoryType = mDevice->FindMemoryType(memoryRequirements.memoryTypeBits,perferredFlags);
	return memoryType;
}
bool Renderer::MemoryTypeNeedsStaging(uint32_t memoryTypeIndex) const
{
	assert(memoryTypeIndex < mDevice->GetPhysicalMemoryProps().memoryTypeCount);
	const VkMemoryPropertyFlags flags = mDevice->GetPhysicalMemoryProps().memoryTypes[memoryTypeIndex].propertyFlags;
	return (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0;
}