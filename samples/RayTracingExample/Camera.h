#pragma once
#include <cstdint>
#include "labgraphics.h"
class _Camera
{
public:
    _Camera();
    ~_Camera();

    void SetViewport(int32_t left, int32_t right, int32_t top, int32_t bottom);
    void SetFovY(float fovy);
    void SetViewPlanes(float nearz, float farz);
    void SetPosition(const Vector3f &pos);

    void LookAt(const Vector3f &pos, const Vector3f &target);
    void Move(float side, float direction);
    void Rotate(float angleX, float angleY);

    float GetNearPlane() const;
    float GetFarPlane() const;
    float GetFovY() const;

    const Matrix4f &GetProjection() const;
    const Matrix4f &GetTransform() const;

    const Vector3f &GetPosition() const;
    const Vector3f &GetDirection() const;
    Vector3f GetUp() ;
    Vector3f GetSide();

private:
    void MakeProjection();
    void MakeTransform();

    int32_t mLeft, mTop, mRight, mBottom;
    float mFovY;
    float mNearZ;
    float mFarZ;
    Vector3f mPosition;
    Vector3f mDirection;
    Quaternionf mRotation;
    Matrix4f mProjection;
    Matrix4f mTransform;
};