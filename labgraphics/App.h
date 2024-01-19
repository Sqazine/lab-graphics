#pragma once
#include "Window.h"
#include "Timer.h"
#include "InputSystem.h"
#include "VK/GraphicsContext.h"
#include "Scene.h"
class App
{
public:
    static App &Instance()
    {
        static App instance;
        return instance;
    }

    App(const App &) = delete;
    App(App &&) = delete;
    App &operator=(const App &) = delete;
    App &operator=(App &&) = delete;

    void Run();
    void Quit();

    Window *GetWindow() const;
    Timer &GetTimer();
    InputSystem &GetInputSystem();
    GraphicsContext* GetGraphicsContext() const;

    void AddScene(Scene *s);

private:
    App() = default;
    ~App() = default;

    void Init();
    void ProcessInput();
    void Update();
    void Render();
    void RenderUI();
    void CleanUp();

    std::unique_ptr<Window> mWindow;
    InputSystem mInputSystem;
    Timer mTimer;
    std::unique_ptr<GraphicsContext> mGraphicsContext;

    std::vector<std::unique_ptr<Scene>> mScenes;

    bool mIsRunning = true;
};