#pragma once
#include "Math/Vector3.h"
#include "Math/Matrix4.h"
#include "Math/Math.hpp"
class Camera
{
public:
	Camera();
	Camera(const Vector3f& position, const Vector3f& target);
	virtual ~Camera();

	virtual void ProcessInput();
	virtual void Update();
	Vector3f mPosition;
	Matrix4f mProjectionMatrix;
	Matrix4f mViewMatrix;
	Vector3f mTarget;

	Vector3f mFront;
	Vector3f mUp;
	Vector3f mRight;
	float mYaw, mPitch, mRoll;
};

class FPSCamera : public Camera
{
public:
	FPSCamera();
	FPSCamera(const Vector3f& position, const Vector3f& target);
	~FPSCamera();
	void ProcessInput() override;

private:
	float mMoveSpeed;
	float mBoostSpeed;
	float mRotateSpeed;
};

class OrbitCamera : public Camera
{
public:
	OrbitCamera();
	~OrbitCamera() {}

	void ProcessInput() override;

private:
	float mRotatepeed;
};


class RmCamera
{
public:
	RmCamera(Vector3f position, Vector3f target, float fov, float aspect);

	~RmCamera() = default;

	Matrix4f GetView();
	Matrix4f GetProjection() const;
	Vector3f GetDirection() const;
	Vector3f GetPosition() const;

	void ProcessInput();

	bool HaveUpdate();
	void Update();

private:
	Matrix4f projection;

	Vector3f position;
	Vector3f target;

	Vector3f front{0, 0, 1};
	Vector3f right{1, 0, 0};
	Vector3f up{0, 1, 0};

	bool isCameraLeft = false;
	bool isCameraRight = false;
	bool isCameraUp = false;
	bool isCameraDown = false;
	bool isCameraFront = false;
	bool isCameraBack = false;
	bool isCameraRotate = false;

	float pitch, yaw, fov, aspect;
};
