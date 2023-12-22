#pragma once
#include <iostream>
#include <vector>
#include <string_view>
#include <cgltf/cgltf.h>
#include <vector>
#include "labgraphics.h"
#include "Material.h"
struct _Vertex
{
    Vector3f position;
    Vector3f normal;
    Vector2f texcoord;

    _Vertex& operator=(const _Vertex& other)
    {
        this->position=other.position;
        this->normal=other.normal;
        this->texcoord=other.texcoord;
        return *this;
    }
};

enum class _MeshType
{
    QUAD,
    SPHERE,
    CUBE,
};

struct _Mesh
{
    std::vector<_Vertex> mVertices;
    std::vector<uint32_t> mIndices;
};

class MeshInstance
{
public:
    Matrix4f modelMat;
    Material material;
    _Mesh mesh;
};

class _Model
{
public:
    _Model(_MeshType type);
    _Model(std::string_view filePath);

    const std::vector<MeshInstance> &GetMeshInstances() const;

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
    std::vector<MeshInstance> mMeshInstances;
};