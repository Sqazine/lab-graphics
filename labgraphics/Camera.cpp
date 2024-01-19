#include "Camera.h"
#include "App.h"

Camera::Camera()
	: mPosition(Vector3f(0.0f, 0.0f, 1.0f)), mTarget(Vector3f(0.0f))
{
	mFront = mTarget - mPosition;
	mRight = Vector3f::Normalize(Vector3f::Cross(mFront, Vector3f(0.0f, 1.0f, 0.0f)));
	mUp = Vector3f::Normalize(Vector3f::Cross(mRight, mFront));

	mProjectionMatrix = Matrix4f::VKPerspective(60.0f, 960.0 / 540.0, 0.001f, 1000.0f);
}
Camera::Camera(const Vector3f &position, const Vector3f &target)
	: mPosition(position), mTarget(target)
{
	mFront = target - position;
	mRight = Vector3f::Normalize(Vector3f::Cross(mFront, Vector3f(0.0f, 1.0f, 0.0f)));
	mUp = Vector3f::Normalize(Vector3f::Cross(mRight, mFront));
}

Camera::~Camera()
{
}

void Camera::ProcessInput()
{
}

void Camera::Update()
{
	mViewMatrix = Matrix4f::LookAt(mPosition, mTarget, mUp);
}

FPSCamera::FPSCamera()
	: Camera(), mMoveSpeed(10.0f), mRotateSpeed(10.0f), mBoostSpeed(100.0f)
{
}

FPSCamera::FPSCamera(const Vector3f &position, const Vector3f &target)
	: Camera(position, target), mMoveSpeed(10.0f), mRotateSpeed(10.0f), mBoostSpeed(100.0f)
{
	mYaw = -90.0f;
	mPitch = 0.0f;
	mRoll = 0.0f;
}

FPSCamera::~FPSCamera()
{
}

void FPSCamera::ProcessInput()
{
	if (!App::Instance().GetInputSystem().GetMouse().IsReleativeMode())
		App::Instance().GetInputSystem().GetMouse().SetReleativeMode(true);

	float speed = 0;

	float deltaTime = App::Instance().GetTimer().GetDeltaTime();

	if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_LSHIFT) == ButtonState::HOLD)
		speed = (mMoveSpeed + mBoostSpeed) * deltaTime;
	else
		speed = mMoveSpeed * deltaTime;

	if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_W) == ButtonState::HOLD)
		mPosition += mFront * speed;
	if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_S) == ButtonState::HOLD)
		mPosition -= mFront * speed;
	if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_A) == ButtonState::HOLD)
		mPosition -= mRight * speed;
	if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_D) == ButtonState::HOLD)
		mPosition += mRight * speed;
	if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_E) == ButtonState::HOLD)
		mPosition += mUp * speed;
	if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_Q) == ButtonState::HOLD)
		mPosition -= mUp * speed;

	mYaw += App::Instance().GetInputSystem().GetMouse().GetMousePos().x * mRotateSpeed * deltaTime;
	mPitch -= App::Instance().GetInputSystem().GetMouse().GetMousePos().y * mRotateSpeed * deltaTime;

	//我们不希望相机在俯仰时发生视觉倒转的情况,因此要限制俯仰角
	mPitch = Math::Clamp(mPitch, -89.0f, 89.0f);

	mFront = Vector3f::Normalize(Vector3f(cos(Math::ToRadian(mPitch)) * cos(Math::ToRadian(mYaw)), sin(Math::ToRadian(mPitch)), cos(Math::ToRadian(mPitch)) * sin(Math::ToRadian(mYaw))));
	mRight = Vector3f::Normalize(Vector3f::Cross(mFront, Vector3f(0.0f, 1.0f, 0.0f)));
	mUp = Vector3f::Normalize(Vector3f::Cross(mRight, mFront));

	mTarget = mPosition + mFront;
}

OrbitCamera::OrbitCamera()
	: Camera(), mRotatepeed(30.0f)
{
	mPitch = 0.0f;
	mYaw = 90.0f;
	mPosition = Vector3f(0.0f, 0.0f, 10.0f);
}

void OrbitCamera::ProcessInput()
{
	if (!App::Instance().GetInputSystem().GetMouse().IsReleativeMode())
		App::Instance().GetInputSystem().GetMouse().SetReleativeMode(true);

	Vector3f tp = mPosition - mTarget;
	Vector3f tp_nor = Vector3f::Normalize(tp);
	float tp_len = tp.Length();
	float mouse_wheel_len = App::Instance().GetInputSystem().GetMouse().GetMouseScrollWheel().y;

	mPosition = mTarget + tp_nor * (tp_len - mouse_wheel_len * mRotatepeed * App::Instance().GetTimer().GetDeltaTime());

	if (App::Instance().GetInputSystem().GetMouse().GetButtonState(SDL_BUTTON_LEFT) == ButtonState::HOLD)
	{
		Vector2i32 mouse_releative_move = App::Instance().GetInputSystem().GetMouse().GetMousePos();

		mYaw += Math::ToRadian(mouse_releative_move.x * mRotatepeed * App::Instance().GetTimer().GetDeltaTime());
		mPitch += Math::ToRadian(mouse_releative_move.y * mRotatepeed * App::Instance().GetTimer().GetDeltaTime());

		mPitch = Math::Clamp(mPitch, Math::ToRadian(-89.0f), Math::ToRadian(89.0f));

		Vector3f tmp = Vector3f(Math::Cos(mYaw) * Math::Cos(mPitch), Math::Sin(mPitch), Math::Cos(mPitch) * Math::Sin(mYaw));

		mPosition = mTarget + tmp * tp_len;
	}
}

float SPEED = 5.0f;
float SENSITIVITY = 5.0f;

RmCamera::RmCamera(Vector3f position, Vector3f target, float fov, float aspect)
    : yaw(0), pitch(0), position(position), target(target), fov(fov), aspect(aspect)
{
    projection = Matrix4f::VKPerspective(fov, aspect, 0.1f, 1000.0f);

    front = Vector3f::Normalize(target - position);
    right = Vector3f::Normalize(Vector3f::Cross(front, Vector3f::UNIT_Y));
    up = Vector3f::Normalize(Vector3f::Cross(right, front));
}

Matrix4f RmCamera::GetView()
{
    return Matrix4f::LookAt(position, target, up);
}

Matrix4f RmCamera::GetProjection() const
{
    return projection;
}

Vector3f RmCamera::GetDirection() const
{
    return front;
}

Vector3f RmCamera::GetPosition() const
{
    return position;
}

void RmCamera::ProcessInput()
{
    if (App::Instance().GetInputSystem().GetMouse().IsReleativeMode())
    {
        if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_S) == ButtonState::HOLD)
            isCameraBack = true;
        else
            isCameraBack = false;

        if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_W) == ButtonState::HOLD)
            isCameraFront = true;
        else
            isCameraFront = false;

        if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_A) == ButtonState::HOLD)
            isCameraLeft = true;
        else
            isCameraLeft = false;

        if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_D) == ButtonState::HOLD)
            isCameraRight = true;
        else
            isCameraRight = false;

        if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_E) == ButtonState::HOLD)
            isCameraUp = true;
        else
            isCameraUp = false;

        if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_Q) == ButtonState::HOLD)
            isCameraDown = true;
        else
            isCameraDown = false;

        auto m = App::Instance().GetInputSystem().GetMouse().GetMousePos();
        if (m.x == 0.0 || m.y == 0.0)
            isCameraRotate = false;
        else
        {
            isCameraRotate = true;
            yaw += m.x * SENSITIVITY * App::Instance().GetTimer().GetDeltaTime();
            pitch -= m.y * SENSITIVITY * App::Instance().GetTimer().GetDeltaTime();
            pitch = Math::Clamp(pitch, -89.0f, 89.0f);
            front = Vector3f::Normalize(Vector3f(Math::Cos(Math::ToRadian(pitch)) * Math::Cos(Math::ToRadian(yaw)), Math::Sin(Math::ToRadian(pitch)), Math::Cos(Math::ToRadian(pitch)) * Math::Sin(Math::ToRadian(yaw))));
            right = Vector3f::Normalize(Vector3f::Cross(front, Vector3f(0.0f, 1.0f, 0.0f)));
            up = Vector3f::Normalize(Vector3f::Cross(right, front));
        }
    }
}

bool RmCamera::HaveUpdate()
{
    return isCameraBack || isCameraDown || isCameraFront || isCameraLeft || isCameraRight || isCameraUp || isCameraRotate;
}

void RmCamera::Update()
{
    if (isCameraBack)
        position -= front * SPEED * App::Instance().GetTimer().GetDeltaTime();
    if (isCameraFront)
        position += front * SPEED * App::Instance().GetTimer().GetDeltaTime();
    if (isCameraLeft)
        position -= right * SPEED * App::Instance().GetTimer().GetDeltaTime();
    if (isCameraRight)
        position += right * SPEED * App::Instance().GetTimer().GetDeltaTime();
    if (isCameraUp)
        position.y += SPEED * App::Instance().GetTimer().GetDeltaTime();
    if (isCameraDown)
        position.y -= SPEED * App::Instance().GetTimer().GetDeltaTime();

    target = position + front;
}