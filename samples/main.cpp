#include "labgraphics.h"
#include "SceneSph.h"
#include "SceneMandelbrotSetGen.h"
#include "SceneRayTraceTriangle.h"
#include "ImguiScene.h"
#include "PathTracer/RaymanScene.h"
#include "Pbr/PbrScene.h"
class SceneManager : public Scene
{
public:
    SceneManager()
    {
        // mScenes.emplace_back(std::make_unique<SceneSph>());
        // mScenes.emplace_back(std::make_unique<SceneMandelbrotSetGen>());
        // mScenes.emplace_back(std::make_unique<PbrScene>());

        // mScenes.emplace_back(std::make_unique<RaymanScene>(std::string(ASSETS_DIR) + "rayman/scene.json"));

        // mScenes.emplace_back(std::make_unique<SceneImgui>());
        mScenes.emplace_back(std::make_unique<SceneRayTraceTriangle>());
    }
    ~SceneManager() override {}

    void Init() override
    {
        for (auto &s : mScenes)
            s->Init();
    }
    void ProcessInput() override
    {
        mScenes[mSceneIdx]->ProcessInput();
    }
    void Update() override
    {
        mScenes[mSceneIdx]->Update();
    }
    void Render() override
    {
        mScenes[mSceneIdx]->Render();
    }
    void RenderUI() override
    {
        mScenes[mSceneIdx]->RenderUI();
    }
    void CleanUp() override
    {
        mScenes[mSceneIdx]->CleanUp();
    }

private:
    uint32_t mSceneIdx = 0;

    std::vector<std::unique_ptr<Scene>> mScenes;
};

int main(int argc, char **argv)
{
    App::Instance().AddScene(new SceneManager());
    App::Instance().Run();

    return 0;
}