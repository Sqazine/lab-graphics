#pragma once
#include <iostream>
#include <vector>
#include <string_view>
#include <cgltf/cgltf.h>
#include <vector>
#include "labgraphics.h"
struct PbrVertex
{
    Vector3f position;
    Vector3f normal;
    Vector2f texcoord;

    PbrVertex &operator=(const PbrVertex &other)
    {
        this->position = other.position;
        this->normal = other.normal;
        this->texcoord = other.texcoord;
        return *this;
    }
};

enum class PbrMeshType
{
    QUAD,
    SPHERE,
    CUBE,
};

struct PbrMesh
{
    std::vector<PbrVertex> mVertices;
    std::vector<uint32_t> mIndices;
};

class PbrMeshInstance
{
public:
    Matrix4f modelMat;
    PbrMesh mesh;
};

class PbrModel
{
public:
    PbrModel(PbrMeshType type);
    PbrModel(std::string_view filePath);

    const std::vector<PbrMeshInstance> &GetMeshInstances() const;

    const Matrix4f &GetModelMat();

private:
    void CreateBuiltInQuad();
    void CreateBuiltInCube();
    void CreateBuiltInSphere();

    cgltf_data *LoadGLTFFile(std::string_view path);
    void FreeGLTFFile(cgltf_data *data);

    Matrix4f GetMeshLocalModelMatrix(const cgltf_node &n);
    int32_t GetNodeIndex(cgltf_node *target, cgltf_node *allNodes, uint32_t numNodes);
    std::vector<float> GetScalarValues(uint32_t compCount, const cgltf_accessor &inAccessor);

    void LoadMeshes(cgltf_data *data);

    Matrix4f mModelMat;
    std::vector<PbrMeshInstance> mMeshInstances;
};