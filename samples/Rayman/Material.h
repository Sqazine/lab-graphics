#pragma once
#include "Math/Vector4.h"
#include "Math/Vector3.h"

enum LightType
{
	QuadLight,
	SphereLight
};

struct Light
{
	alignas(16) Vector3f position{};
	alignas(16) Vector3f emission{};
	alignas(16) Vector3f u{};
	alignas(16) Vector3f v{};
	alignas(4) float area{};
	alignas(4) int32_t type{};
	alignas(4) float radius{};
};

struct Material
{
	Material()
	{
		albedoColor = Vector3f(1.0f, 1.0f, 1.0f);
		emissionColor = Vector3f(0.0f, 0.0f, 0.0f);
		specularColor = Vector3f(1.0f, 1.0f, 1.0f);
		clearcoatColor = Vector3f(1.0f, 1.0f, 1.0f);
		extinctionColor = Vector3f(1.0f, 1.0f, 1.0f);

		metallic = 0.1f;
		roughness = 0.2f;
		bumpiness = 1.0f;
		emissionIntensity = 1.0f;

		sheen = 0.0f;
		sheenTint = 0.0f;
		clearcoat = 0.0f;
		clearcoatGloss = 0.0f;

		transmission = 0.0f;
		ior = 1.001f;
		attenuationDistance = 0.0f;
		thickness = 0.0f;

		specular = 0.5f;
		specularTint = 0.0f;
		subsurface = 0.0f;

		albedoTexID = -1;
		specularTexID = -1;
		metallicTexID = -1;
		roughnessTexID = -1;

		normalTexID = -1;
		emissionTexID = -1;
		opacityTexID = -1;
	}

	alignas(16) Vector3f albedoColor;
	alignas(16) Vector3f emissionColor;
	alignas(16) Vector3f specularColor;
	alignas(16) Vector3f clearcoatColor;
	alignas(16) Vector3f extinctionColor;

	alignas(4) float metallic;
	alignas(4) float roughness;
	alignas(4) float bumpiness;
	alignas(4) float emissionIntensity;

	alignas(4) float sheen;
	alignas(4) float sheenTint;
	alignas(4) float clearcoat;
	alignas(4) float clearcoatGloss;

	alignas(4) float transmission;
	alignas(4) float ior;
	alignas(4) float attenuationDistance;
	alignas(4) float thickness;

	alignas(4) float specular;
	alignas(4) float specularTint;
	alignas(4) float subsurface;

	alignas(4) int32_t albedoTexID;
	alignas(4) int32_t specularTexID;
	alignas(4) int32_t metallicTexID;
	alignas(4) int32_t roughnessTexID;

	alignas(4) int32_t normalTexID;
	alignas(4) int32_t emissionTexID;
	alignas(4) int32_t opacityTexID;
};
