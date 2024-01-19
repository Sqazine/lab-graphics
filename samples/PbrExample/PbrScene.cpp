#include "PbrScene.h"
#include "App.h"
#include "InputSystem.h"

void PbrScene::Init()
{
	mPbrRenderer.Init(4);
}

void PbrScene::ProcessInput()
{
	mCamera.ProcessInput();

	float deltaTime = App::Instance().GetTimer().GetDeltaTime();

	float rotSpeed = 10.0f;

	if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_UP) == ButtonState::HOLD)
		mPitch += rotSpeed * deltaTime;
	if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_DOWN) == ButtonState::HOLD)
		mPitch -= rotSpeed * deltaTime;
	if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_LEFT) == ButtonState::HOLD)
		mYaw -= rotSpeed * deltaTime;
	if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_RIGHT) == ButtonState::HOLD)
		mYaw += rotSpeed * deltaTime;
}

void PbrScene::Update()
{
	mCamera.Update();
	mSkyboxRotationMatrix = Matrix4f::Rotate(Vector3f::UNIT_X, Math::ToRadian(mPitch)) * Matrix4f::Rotate(Vector3f::UNIT_Y, Math::ToRadian(mYaw));
}


void PbrScene::Render()
{
	mPbrRenderer.Render(*this);
}