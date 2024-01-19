#pragma once
#include "labgraphics.h"
#include "Renderer.h"

class PbrScene:public Scene
{
public:
	void Init() override;
	void ProcessInput() override;
	void Update() override;
	void Render() override;

	Renderer mPbrRenderer;

	Matrix4f mSkyboxRotationMatrix;

	float mYaw, mPitch, mRoll;
	OrbitCamera mCamera;
	PbrLight mLights[LIGHT_NUM];
};