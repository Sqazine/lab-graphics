#pragma once
#include <vector>
#include <SDL.h>
#include <memory>
#include "Math/Vector2.h"

enum class ButtonState
{
	NONE,
	PRESS,
	RELEASE,
	HOLD
};

class KeyboardState
{
public:
	KeyboardState(const KeyboardState &) = delete;
	KeyboardState(KeyboardState &&) = delete;
	KeyboardState &operator=(const KeyboardState &) = delete;
	KeyboardState &operator=(KeyboardState &&) = delete;

	bool GetKeyValue(SDL_Scancode keyCode) const;
	ButtonState GetKeyState(SDL_Scancode keyCode) const;

private:
	friend class InputSystem;
	KeyboardState();
	~KeyboardState();

	const uint8_t *mCurKeyState;
	uint8_t *mPreKeyState;
};

class MouseState
{
public:
	MouseState(const MouseState &) = delete;
	MouseState(MouseState &&) = delete;
	MouseState &operator=(const MouseState &) = delete;
	MouseState &operator=(MouseState &&) = delete;

	bool GetButtonValue(int button) const;
	ButtonState GetButtonState(int button) const;
	Vector2i32 GetMousePos() const;
	Vector2i32 GetReleativeMove() const;
	Vector2i32 GetMouseScrollWheel() const;

	void SetReleativeMode(bool isActive);
	bool IsReleativeMode() const;

private:
	friend class InputSystem;
	MouseState();
	~MouseState();

	bool mIsRelative;
	Vector2i32 mCurPos;
	Vector2i32 mPrePos;
	Vector2i32 mMouseScrollWheel;
	uint32_t mCurButtons;
	uint32_t mPreButtons;
};

class InputSystem
{
public:
	InputSystem(const InputSystem &) = delete;
	InputSystem(InputSystem &&) = delete;
	InputSystem &operator=(const InputSystem &) = delete;
	InputSystem &operator=(InputSystem &&) = delete;

	KeyboardState &GetKeyboard();
	MouseState &GetMouse();

	bool IsWindowCloseButtonClick() const
	{
		return mIsWindowCloseButtonClick;
	}

	bool IsWindowMaxButtonClick() const
	{
		return mIsWindowMaxButtonClick;
	}

	bool IsWindowMinButtonClick() const
	{
		return mIsWindowMinButtonClick;
	}

	bool IsWindowResize() const
	{
		return mIsWindowResize;
	}

private:
	InputSystem() = default;
	~InputSystem() = default;

	KeyboardState mKeyboardState;
	MouseState mMouseState;

	friend class App;

	void Init();
	void PostUpdate();
	void ProcessInput();
	void PreUpdate();

	bool mIsWindowCloseButtonClick;
	bool mIsWindowMaxButtonClick;
	bool mIsWindowMinButtonClick;
	bool mIsWindowResize;
};