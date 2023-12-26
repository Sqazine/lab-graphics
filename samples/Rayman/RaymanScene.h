#pragma once

#include <map>
#include <memory>
#include <vector>
#include <string>
#include <filesystem>
#include <future>
#include "labgraphics.h"
#include "Texture.h"
#include "Material.h"
#include "Model.h"
#include "HDRLoader.h"

class Device;
class Buffer;
class RmCommandPool;
class Image;

class RaymanScene:public Scene
{
public:
	RaymanScene(std::string_view filePath);
	~RaymanScene();

	void LoadFromFile(std::string_view filePath);

	void AddHDR(HDRData *hdr);

	int AddTexture(ImageData *texture);

	void SetCamera(Vector3f position, Vector3f target, float fov, float aspect);
	int AddMesh(MeshDef *mesh);
	int AddMaterial(Material material);
	int AddLight(Light light);

	int AddMeshInstance(MeshInstance meshInstance);

	const std::vector<std::unique_ptr<MeshDef>> &GetMeshes() const
	{
		return meshes;
	}

	const std::vector<MeshInstance> &GetMeshInstances() const
	{
		return meshInstances;
	}

	RmCamera &GetCamera() const
	{
		return *camera;
	}

	uint32_t GetLightsSize() const
	{
		return (uint32_t)lights.size();
	}

	float GetHDRResolution() const
	{
		return hdrResolution;
	}

	void Init();
	void ProcessInput();
	void Update();
	void Render();

	float hdrMultiplier = 1.0f;

private:
	friend class RtxRayTraceScene;

	void PrintInfo() const
	{
		std::cout << "[SCENE ANALYZER] Scene info:" << std::endl;
		std::cout << "	         Meshes:    " << meshes.size() << std::endl;
		std::cout << "	         Instances: " << meshInstances.size() << std::endl;
		std::cout << "	         Textures:  " << textureDatas.size() << std::endl;
		std::cout << "	         Lights:    " << lights.size() << std::endl;
		std::cout << "	         Materials: " << materials.size() << std::endl;
	}

	std::unique_ptr<class RtxRayTraceScene> mRtxRayTraceScene;

	std::unique_ptr<RmCamera> camera;

	std::vector<std::unique_ptr<MeshDef>> meshes;
	std::vector<std::unique_ptr<ImageData>> textureDatas;

	std::unique_ptr<ImageData> hdrColumns;
	std::unique_ptr<ImageData> hdrConditional;
	std::unique_ptr<ImageData> hdrMarginal;

	std::vector<MeshInstance> meshInstances;
	std::vector<Material> materials;
	std::vector<Light> lights;

	float hdrResolution{};
};