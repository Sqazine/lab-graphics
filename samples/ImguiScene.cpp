#include "ImguiScene.h"

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

    // Setup backend capabilities flags
    ImGuiIO &io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_vulkan";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset; // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

     // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

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
    init_info.MinImageCount = App::Instance().GetGraphicsContext()->GetSwapChain()->GetImageCount();
    init_info.ImageCount = mMainWindowData.ImageCount;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, mMainWindowData.RenderPass);
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
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame(App::Instance().GetWindow()->GetHandle());
    ImGui::NewFrame();

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (mShowDemoWindow)
        ImGui::ShowDemoWindow(&mShowDemoWindow);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");          // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &mShowDemoWindow); // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &mShowAnotherWindow);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);             // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float *)&clear_color); // Edit 3 floats representing a color

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
        wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
        wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
        wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
        wd->ClearValue.color.float32[3] = clear_color.w;
        FrameRender(wd, draw_data);
        FramePresent(wd);
    }
}
void SceneImgui::CleanUp()
{
}