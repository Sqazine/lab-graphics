#include "InputSystem.h"
#include <memory>
#include "App.h"

KeyboardState::KeyboardState()
{
}
KeyboardState::~KeyboardState()
{
}
bool KeyboardState::GetKeyValue(SDL_Scancode keyCode) const
{
	return mCurKeyState[keyCode] == 1 ? true : false;
}

ButtonState KeyboardState::GetKeyState(SDL_Scancode keyCode) const
{
	if (mPreKeyState[keyCode] == 0)
	{
		if (mCurKeyState[keyCode] == 0)
			return ButtonState::NONE;
		else
			return ButtonState::PRESS;
	}
	else
	{
		if (mCurKeyState[keyCode] == 0)
			return ButtonState::RELEASE;
		else
			return ButtonState::HOLD;
	}
}

MouseState::MouseState()
	: mCurPos(Vector2i32::ZERO), mPrePos(Vector2i32::ZERO), mMouseScrollWheel(Vector2i32::ZERO), mCurButtons(0), mPreButtons(0)
{
}
MouseState::~MouseState()
{
}
bool MouseState::GetButtonValue(int button) const
{
	return (mCurButtons & SDL_BUTTON(button)) == 1;
}

ButtonState MouseState::GetButtonState(int button) const
{
	if ((mPreButtons & SDL_BUTTON(button)) == 0)
	{
		if ((mPreButtons & SDL_BUTTON(button)) == 0)
			return ButtonState::NONE;
		else
			return ButtonState::PRESS;
	}
	else
	{
		if ((mPreButtons & SDL_BUTTON(button)) == 0)
			return ButtonState::RELEASE;
		else
			return ButtonState::HOLD;
	}
}

Vector2i32 MouseState::GetMousePos() const
{
	return mCurPos;
}

Vector2i32 MouseState::GetReleativeMove() const
{
	return mCurPos - mPrePos;
}

Vector2i32 MouseState::GetMouseScrollWheel() const
{
	return mMouseScrollWheel;
}

void MouseState::SetReleativeMode(bool isActive)
{
	mIsRelative = isActive;
	if (isActive)
		SDL_SetRelativeMouseMode(SDL_TRUE);
	else
		SDL_SetRelativeMouseMode(SDL_FALSE);
}

bool MouseState::IsReleativeMode() const
{
	return mIsRelative;
}

void InputSystem::Init()
{

	// 获取SDL中键盘状态
	mKeyboardState.mCurKeyState = SDL_GetKeyboardState(nullptr);
	// 清空前一帧键盘状态的值（游戏开始前没有状态）
	mKeyboardState.mPreKeyState = new uint8_t[SDL_NUM_SCANCODES];
	memset(mKeyboardState.mPreKeyState, 0, SDL_NUM_SCANCODES);
}

void InputSystem::PreUpdate()
{
	memcpy_s(mKeyboardState.mPreKeyState, SDL_NUM_SCANCODES, mKeyboardState.mCurKeyState, SDL_NUM_SCANCODES);
	mMouseState.mPreButtons = mMouseState.mCurButtons;
	mMouseState.mPrePos = mMouseState.mCurPos;
	mMouseState.mMouseScrollWheel = Vector2i32::ZERO;

	mIsWindowMinButtonClick = false;
	mIsWindowMaxButtonClick = false;
	mIsWindowCloseButtonClick = false;
	mIsWindowResize = false;
}

void InputSystem::PostUpdate()
{
	Vector2i32 p = Vector2i32::ZERO;
	// 更新当前帧的鼠标按键的状态
	if (!mMouseState.mIsRelative) // 获取鼠标光标位置的绝对位置
		mMouseState.mCurButtons = SDL_GetMouseState(&p.x, &p.y);
	else // 获取鼠标光标的相对位置
		mMouseState.mCurButtons = SDL_GetRelativeMouseState(&p.x, &p.y);
	// 更新当前帧的鼠标光标位置
	mMouseState.mCurPos = p;
}

void InputSystem::ProcessInput()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{

		switch (event.type)
		{
		case SDL_MOUSEWHEEL:
			mMouseState.mMouseScrollWheel = Vector2i32(event.wheel.x, static_cast<float>(event.wheel.y));
			break;
		case SDL_QUIT:
			App::Instance().Quit();
			break;
		case SDL_WINDOWEVENT:
		{
			switch (event.window.event)
			{
			case SDL_WINDOWEVENT_MINIMIZED:
				mIsWindowMinButtonClick = true;
				break;
			case SDL_WINDOWEVENT_MAXIMIZED:
				mIsWindowMaxButtonClick = true;
				break;
			case SDL_WINDOWEVENT_CLOSE:
				mIsWindowCloseButtonClick = true;
				break;
			case SDL_WINDOWEVENT_RESIZED:
				mIsWindowResize = true;
			default:
				break;
			}
		}
		default:
			break;
		}
	}
}

KeyboardState &InputSystem::GetKeyboard()
{
	return mKeyboardState;
}

MouseState &InputSystem::GetMouse()
{
	return mMouseState;
}
