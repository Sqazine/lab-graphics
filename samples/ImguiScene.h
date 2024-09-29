#pragma once
#include <memory>
#include "labgraphics.h"
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/backends/imgui_impl_sdl.h>
class SceneImgui : public Scene
{
public:
    void Init() override;
    void ProcessInput() override;
    void Update() override;
    void Render() override;
    void RenderUI() override;
    void CleanUp() override;

private:
    bool mShowDemoWindow = true;
    bool mShowAnotherWindow = false;
    ImVec4 mClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    std::unique_ptr<DescriptorTable> mDescriptorTable;

    std::unique_ptr<RasterPass> mImguiPass;
};