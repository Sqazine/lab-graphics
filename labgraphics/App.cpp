#include "App.h"
#include "Logger.h"
void App::Run()
{
	Init();
	while (mIsRunning)
	{
		ProcessInput();
		Update();
		Render();
		RenderUI();
	}
	CleanUp();
}

void App::Quit()
{
	mIsRunning = false;
}

Window *App::GetWindow() const
{
	return mWindow.get();
}
Timer &App::GetTimer()
{
	return mTimer;
}

InputSystem &App::GetInputSystem()
{
	return mInputSystem;
}

GraphicsContext *App::GetGraphicsContext() const
{
	return mGraphicsContext.get();
}

void App::AddScene(Scene *s)
{
	mScenes.emplace_back(s);
}

void App::Init()
{
	Logger::Init();
	mWindow = std::make_unique<Window>();
	mGraphicsContext = std::make_unique<GraphicsContext>();
	mInputSystem.Init();

	for (const auto &scene : mScenes)
		scene->Init();

	if (!mWindow->IsVisible())
		mWindow->Show();
}
void App::ProcessInput()
{
	mInputSystem.PreUpdate();

	mInputSystem.ProcessInput();

	if (mInputSystem.GetKeyboard().GetKeyState(SDL_SCANCODE_ESCAPE) == ButtonState::PRESS)
		Quit();

	for (const auto &scene : mScenes)
		scene->ProcessInput();

	mInputSystem.PostUpdate();
}
void App::Update()
{
	mTimer.Update();

	for (const auto &scene : mScenes)
		scene->Update();
}
void App::Render()
{
	for (const auto &scene : mScenes)
		scene->Render();
}

void App::RenderUI()
{
	for (const auto &scene : mScenes)
		scene->RenderUI();
}

void App::CleanUp()
{
	for (const auto &scene : mScenes)
		scene->CleanUp();

	mWindow.reset(nullptr);
}