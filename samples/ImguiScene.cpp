#include "ImguiScene.h"
#include "App.h"

static void check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

void SceneImgui::Init()
{
    mDescriptorTable = std::make_unique<DescriptorTable>(*App::Instance().GetGraphicsContext()->GetDevice());
    mDescriptorTable->GetPool()->AddPoolDesc(DescriptorType::SAMPLER, 1000);
    mDescriptorTable->GetPool()->AddPoolDesc(DescriptorType::COMBINED_IMAGE_SAMPLER, 1000);
    mDescriptorTable->GetPool()->AddPoolDesc(DescriptorType::SAMPLED_IMAGE, 1000);
    mDescriptorTable->GetPool()->AddPoolDesc(DescriptorType::STORAGE_IMAGE, 1000);
    mDescriptorTable->GetPool()->AddPoolDesc(DescriptorType::UNIFORM_TEXEL_BUFFER, 1000);
    mDescriptorTable->GetPool()->AddPoolDesc(DescriptorType::STORAGE_TEXEL_BUFFER, 1000);
    mDescriptorTable->GetPool()->AddPoolDesc(DescriptorType::UNIFORM_BUFFER, 1000);
    mDescriptorTable->GetPool()->AddPoolDesc(DescriptorType::STORAGE_BUFFER, 1000);
    mDescriptorTable->GetPool()->AddPoolDesc(DescriptorType::UNIFORM_BUFFER_DYNAMIC, 1000);
    mDescriptorTable->GetPool()->AddPoolDesc(DescriptorType::STORAGE_BUFFER_DYNAMIC, 1000);
    mDescriptorTable->GetPool()->AddPoolDesc(DescriptorType::INPUT_ATTACHMENT, 1000);

    auto inFlightFrameCount = (int32_t)App::Instance().GetGraphicsContext()->GetSwapChain()->GetImages().size();
    mImguiPass = std::make_unique<RasterPass>(inFlightFrameCount);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForVulkan(App::Instance().GetWindow()->GetHandle());
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = App::Instance().GetGraphicsContext()->GetInstance()->GetHandle();
    init_info.PhysicalDevice = App::Instance().GetGraphicsContext()->GetDevice()->GetPhysicalHandle();
    init_info.Device = App::Instance().GetGraphicsContext()->GetDevice()->GetHandle();
    init_info.QueueFamily = App::Instance().GetGraphicsContext()->GetDevice()->GetQueueFamilyIndices().graphicsFamily.value();
    init_info.Queue = App::Instance().GetGraphicsContext()->GetDevice()->GetGraphicsQueue()->GetHandle();
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = mDescriptorTable->GetPool()->GetHandle();
    init_info.Allocator = nullptr;
    init_info.MinImageCount = App::Instance().GetGraphicsContext()->GetSwapChain()->GetImageViews().size();
    init_info.ImageCount = App::Instance().GetGraphicsContext()->GetSwapChain()->GetImageViews().size();
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, App::Instance().GetGraphicsContext()->GetSwapChain()->GetDefaultRenderPass()->GetHandle());

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // io.Fonts->AddFontDefault();
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    // ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    // IM_ASSERT(font != NULL);

    // Upload Fonts
    {
        auto cmdBuffer = App::Instance().GetGraphicsContext()->GetDevice()->GetRasterCommandPool()->CreatePrimaryCommandBuffer();

        cmdBuffer->Record([&]()
                          { ImGui_ImplVulkan_CreateFontsTexture(cmdBuffer->GetHandle()); });

        cmdBuffer->Submit();

        App::Instance().GetGraphicsContext()->GetDevice()->WaitIdle();
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
}
void SceneImgui::ProcessInput()
{
}
void SceneImgui::Update()
{
}

void SceneImgui::Render()
{
}
void SceneImgui::RenderUI()
{
    ImGui_ImplSDL2_NewFrame(App::Instance().GetWindow()->GetHandle());
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (mShowDemoWindow)
        ImGui::ShowDemoWindow(&mShowDemoWindow);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");         // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &mShowDemoWindow); // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &mShowAnotherWindow);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);             // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float *)&mClearColor); // Edit 3 floats representing a color

        if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (mShowAnotherWindow)
    {
        ImGui::Begin("Another Window", &mShowAnotherWindow); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            mShowAnotherWindow = false;
        ImGui::End();
    }

    // Rendering
    ImGui::Render();
    ImDrawData *draw_data = ImGui::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
    if (!is_minimized)
    {
        mImguiPass->RecordCurrentCommand([&](RasterCommandBuffer *rasterCmd,size_t curFrameIdx)
                                         {
            rasterCmd->BeginRenderPass(App::Instance().GetGraphicsContext()->GetSwapChain()->GetDefaultRenderPass()->GetHandle(),
            App::Instance().GetGraphicsContext()->GetSwapChain()->GetDefaultFrameBuffers()[curFrameIdx]->GetHandle(),
            App::Instance().GetGraphicsContext()->GetSwapChain()->GetRenderArea(),
            {VkClearValue{mClearColor.x, mClearColor.y, mClearColor.z,   mClearColor.w}},
            VK_SUBPASS_CONTENTS_INLINE);
            // Record dear imgui primitives into command buffer
            ImGui_ImplVulkan_RenderDrawData(draw_data, rasterCmd->GetHandle());
            rasterCmd->EndRenderPass(); });
        mImguiPass->Render();
    }
}

void SceneImgui::CleanUp()
{
    App::Instance().GetGraphicsContext()->GetDevice()->WaitIdle();
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}