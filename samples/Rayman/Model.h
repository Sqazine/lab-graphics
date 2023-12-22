#pragma once
#include <string>
#include <vector>
#include "Math/Vector3.h"
#include "Math/Vector2.h"
#include "Math/Matrix4.h"

struct Vertex
{
	alignas(4) Vector3f position;
	alignas(4) Vector3f normal;
	alignas(4) Vector2f texcoord;
	alignas(4) int32_t materialId;
};
class MeshDef
{
public:
	MeshDef(std::vector<Vertex> vertices, std::vector<uint32_t> indices)
		: vertices(vertices), indices(indices)
	{
	}

	MeshDef *Clone()
	{
		return new MeshDef(vertices, indices);
	}
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

class MeshInstance
{
public:
	MeshInstance(int meshId, Matrix4f transformation, int matId)
		: modelTransform(transformation), materialId(matId), meshId(meshId) {}

	~MeshInstance() = default;

	Matrix4f modelTransform = Matrix4f::IDENTITY;
	int materialId;
	int meshId;
};

struct ModelDef
{
	void LoadFromFile(const std::string &path);
	std::vector<MeshDef *> meshes;
	std::vector<std::string> matNames;
};