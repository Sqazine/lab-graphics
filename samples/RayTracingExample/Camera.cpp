#include "Camera.h"
#include "labgraphics.h"
_Camera::_Camera()
    : mFovY(60.0f), mNearZ(1.0f), mFarZ(1000.0f), mPosition(0.0f, 0.0f, 0.0f), mDirection(0.0f, 0.0f, 1.0f)
{
}
_Camera::~_Camera() {}

void _Camera::SetViewport(int32_t left, int32_t right, int32_t top, int32_t bottom)
{
    mLeft = left;
    mRight = right;
    mTop = top;
    mBottom = bottom;

    MakeProjection();
}
void _Camera::SetFovY(float fovy)
{
    mFovY = fovy;
    MakeProjection();
}
void _Camera::SetViewPlanes(float nearz, float farz)
{
    mNearZ = nearz;
    mFarZ = farz;
    MakeProjection();
}

void _Camera::SetPosition(const Vector3f &pos)
{
    mPosition = pos;
    MakeTransform();
}

void _Camera::LookAt(const Vector3f &pos, const Vector3f &target)
{
    mPosition = pos;
    mDirection = Vector3f::Normalize(target - pos);
    MakeTransform();
}
void _Camera::Move(float side, float direction)
{
    Vector3f cameraSide =  Vector3f::Normalize(Vector3f::Cross(mDirection, Vector3f(0.0f, 1.0f, 0.0f)));
    mPosition += cameraSide * side;
    mPosition += mDirection * direction;
    MakeTransform();
}
void _Camera::Rotate(float angleX, float angleY)
{
    Vector3f side =  Vector3f::Cross(mDirection, Vector3f(0.0f, 1.0f, 0.0f));
    Quaternionf pitchQ(side,Math::ToRadian(angleY));
    Quaternionf headingQ(Vector3f::UNIT_Y, Math::ToRadian(angleX));

    Quaternionf temp = Quaternionf::Normalize(pitchQ * headingQ);

    mDirection =  Vector3f::Normalize(temp * mDirection);

    MakeTransform();
}

float _Camera::GetNearPlane() const
{
    return mNearZ;
}
float _Camera::GetFarPlane() const
{
    return mFarZ;
}
float _Camera::GetFovY() const
{
    return mFovY;
}

const Matrix4f &_Camera::GetProjection() const
{
    return mProjection;
}
const Matrix4f &_Camera::GetTransform() const
{
    return mTransform;
}

const Vector3f &_Camera::GetPosition() const
{
    return mPosition;
}
const Vector3f &_Camera::GetDirection() const
{
    return mDirection;
}
Vector3f _Camera::GetUp()
{
    return Vector3f(mTransform[0][1], mTransform[1][1], mTransform[2][1]);
}
Vector3f _Camera::GetSide()
{
    return Vector3f(mTransform[0][0], mTransform[1][0], mTransform[2][0]);
}

void _Camera::MakeProjection()
{
    float aspect = float(mRight - mLeft) / (mBottom - mTop);
    mProjection = Matrix4f::VKPerspective(mFovY, aspect, mNearZ, mFarZ);
}
void _Camera::MakeTransform()
{
    mTransform = Matrix4f::LookAt(mPosition, mPosition + mDirection, Vector3f(0.0f, 1.0f, 0.0f));
}
