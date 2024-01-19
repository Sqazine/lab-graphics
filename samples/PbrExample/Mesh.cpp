#include "Mesh.h"
#include <cgltf/cgltf.h>

PbrModel::PbrModel(PbrMeshType type)
{
    switch (type)
    {
    case PbrMeshType::QUAD:
        CreateBuiltInQuad();
        break;
    case PbrMeshType::CUBE:
        CreateBuiltInCube();
        break;
    case PbrMeshType::SPHERE:
        CreateBuiltInSphere();
        break;
    default:
        std::cout << "MODEL::ERROR:failed to create model" << std::endl;
        break;
    }
}

PbrModel::PbrModel(std::string_view filePath)
{
    auto cgltfData = LoadGLTFFile(filePath);
    LoadMeshes(cgltfData);
    FreeGLTFFile(cgltfData);
}

cgltf_data *PbrModel::LoadGLTFFile(std::string_view path)
{
    cgltf_options options;
    memset(&options, 0, sizeof(cgltf_options));
    cgltf_data *data = nullptr;
    cgltf_result result = cgltf_parse_file(&options, path.data(), &data);
    if (result != cgltf_result_success)
    {
        std::cout << "Could not load:" << path << std::endl;
        return 0;
    }
    result = cgltf_load_buffers(&options, data, path.data());
    if (result != cgltf_result_success)
    {
        cgltf_free(data);
        std::cout << "Could not load:" << path << std::endl;
        return 0;
    }
    result = cgltf_validate(data);
    if (result != cgltf_result_success)
    {
        cgltf_free(data);
        std::cout << "Invalid file:" << path << std::endl;
        return 0;
    }
    return data;
}
void PbrModel::FreeGLTFFile(cgltf_data *data)
{
    if (!data)
        std::cout << "WARNING:Can't free null data" << std::endl;
    else
        cgltf_free(data);
}

Matrix4f PbrModel::GetMeshLocalModelMatrix(const cgltf_node &n)
{
    Matrix4f mat4(n.matrix[0], n.matrix[1], n.matrix[2], n.matrix[3],
                  n.matrix[4], n.matrix[5], n.matrix[6], n.matrix[7],
                  n.matrix[8], n.matrix[9], n.matrix[10], n.matrix[11],
                  n.matrix[12], n.matrix[13], n.matrix[14], n.matrix[15]);
    return mat4;
}

int32_t PbrModel::GetNodeIndex(cgltf_node *target, cgltf_node *allNodes, uint32_t numNodes)
{
    if (target == 0)
        return -1;

    for (uint32_t i = 0; i < numNodes; ++i)
    {
        if (target == &allNodes[i])
            return (int)i;
    }
    return -1;
}

std::vector<float> PbrModel::GetScalarValues(uint32_t compCount, const cgltf_accessor &inAccessor)
{
    std::vector<float> out;
    out.resize(inAccessor.count * compCount);
    for (cgltf_size i = 0; i < inAccessor.count; ++i)
        cgltf_accessor_read_float(&inAccessor, i, &out[i * compCount], compCount);
    return out;
}

void PbrModel::LoadMeshes(cgltf_data *data)
{
    cgltf_node *nodes = data->nodes;
    uint32_t nodeCount = data->nodes_count;
    for (uint32_t i = 0; i < nodeCount; ++i)
    {
        cgltf_node node = nodes[i];
        if (node.mesh == 0)
            continue;
        int numPrims = node.mesh->primitives_count;

        PbrMesh mesh;
        for (int j = 0; j < numPrims; ++j)
        {
            cgltf_primitive primitive = node.mesh->primitives[j];
            uint32_t ac = primitive.attributes_count;

            std::vector<Vector3f> positions;
            std::vector<Vector3f> normals;
            std::vector<Vector2f> texcoords;

            for (uint32_t k = 0; k < ac; ++k)
            {
                cgltf_attribute *attribute = &primitive.attributes[k];
                cgltf_accessor accessor = *attribute->data;
                uint32_t componentCount = 0;

                if (accessor.type == cgltf_type_vec2)
                    componentCount = 2;
                else if (accessor.type == cgltf_type_vec3)
                    componentCount = 3;
                else if (accessor.type == cgltf_type_vec4)
                    componentCount = 4;

                std::vector<float> values = GetScalarValues(componentCount, accessor);
                uint32_t accessorCount = accessor.count;

                for (uint32_t i = 0; i < accessorCount; ++i)
                {
                    int index = i * componentCount;
                    switch (attribute->type)
                    {
                    case cgltf_attribute_type_position:
                        positions.emplace_back(Vector3f(values[index + 0], values[index + 1], values[index + 2]));
                        break;
                    case cgltf_attribute_type_texcoord:
                        texcoords.emplace_back(Vector2f(values[index + 0], values[index + 1]));
                        break;
                    case cgltf_attribute_type_normal:
                    {
                        Vector3f n = Vector3f(values[index + 0], values[index + 1], values[index + 2]);
                        if (n.SquareLength() < 0.00001f)
                            n = Vector3f(0.0f, 1.0, 0.0f);
                        normals.emplace_back(n);
                        break;
                    }
                    }
                }
            }
            for (int32_t i = 0; i < positions.size(); ++i)
            {
                PbrVertex v;
                v.position = positions[i];
                v.normal = normals[i];
                v.texcoord = texcoords[i];

                mesh.mVertices.emplace_back(v);
            }

            if (primitive.indices != 0)
            {
                for (uint32_t k = 0; k < primitive.indices->count; ++k)
                    mesh.mIndices.emplace_back(cgltf_accessor_read_index(primitive.indices, k));
            }
        }

        auto localModelMat = GetMeshLocalModelMatrix(node);
        PbrMeshInstance meshInstance;
        meshInstance.mesh = mesh;
        meshInstance.modelMat=localModelMat;
        mMeshInstances.emplace_back(meshInstance);
    }
}

void PbrModel::CreateBuiltInQuad()
{
    std::vector<Vector3f> positions =
        {
            Vector3f(-1.0f, 1.0f, 0.0f),
            Vector3f(-1.0f, -1.0f, 0.0f),
            Vector3f(1.0f, -1.0f, 0.0f),
            Vector3f(1.0f, 1.0f, 0.0f)};

    std::vector<Vector3f> normals =
        {
            Vector3f(0.0f, 0.0f, 1.0f),
            Vector3f(0.0f, 0.0f, 1.0f),
            Vector3f(0.0f, 0.0f, 1.0f),
            Vector3f(0.0f, 0.0f, 1.0f)

        };

    std::vector<Vector2f> texcoords =
        {
            Vector2f(0.0f, 1.0f),
            Vector2f(0.0f, 0.0f),
            Vector2f(1.0f, 0.0f),
            Vector2f(1.0f, 1.0f),
        };

    std::vector<Vector3f> tangents = std::vector<Vector3f>(4, Vector3f(1.0, 0.0, 0.0));

    std::vector<uint32_t> indices =
        {
            0, 1, 2,
            0, 2, 3};

    PbrMesh mesh;
    for (int32_t i = 0; i < 4; ++i)
    {
        PbrVertex v;
        v.position = positions[i];
        v.normal = normals[i];
        v.texcoord = texcoords[i];
        mesh.mVertices.emplace_back(v);
    }

    mesh.mIndices = indices;

    PbrMeshInstance instance;
    instance.mesh = mesh;

    mMeshInstances.emplace_back(instance);
}
void PbrModel::CreateBuiltInCube()
{
    std::vector<Vector3f> positions =
        {
            //+z
            Vector3f(-0.5f, 0.5f, 0.5f),
            Vector3f(-0.5f, -0.5f, 0.5f),
            Vector3f(0.5f, -0.5f, 0.5f),
            Vector3f(0.5f, 0.5f, 0.5f),
            //-z
            Vector3f(0.5f, 0.5f, -0.5f),
            Vector3f(0.5f, -0.5f, -0.5f),
            Vector3f(-0.5f, -0.5f, -0.5f),
            Vector3f(-0.5f, 0.5f, -0.5f),
            //+x
            Vector3f(0.5f, 0.5f, 0.5f),
            Vector3f(0.5f, -0.5f, 0.5f),
            Vector3f(0.5f, -0.5f, -0.5f),
            Vector3f(0.5f, 0.5f, -0.5f),
            //-x
            Vector3f(-0.5f, 0.5f, -0.5f),
            Vector3f(-0.5f, -0.5f, -0.5f),
            Vector3f(-0.5f, -0.5f, 0.5f),
            Vector3f(-0.5f, 0.5f, 0.5f),
            //+y
            Vector3f(-0.5f, 0.5f, -0.5f),
            Vector3f(-0.5f, 0.5f, 0.5f),
            Vector3f(0.5f, 0.5f, 0.5f),
            Vector3f(0.5f, 0.5f, -0.5f),
            //-y
            Vector3f(-0.5f, -0.5f, 0.5f),
            Vector3f(-0.5f, -0.5f, -0.5f),
            Vector3f(0.5f, -0.5f, -0.5f),
            Vector3f(0.5f, -0.5f, 0.5f),
        };

    std::vector<Vector3f> normals =
        {
            //+z
            Vector3f(0.0f, 0.0f, 1.0f),
            Vector3f(0.0f, 0.0f, 1.0f),
            Vector3f(0.0f, 0.0f, 1.0f),
            Vector3f(0.0f, 0.0f, 1.0f),
            //-z
            Vector3f(0.0f, 0.0f, -1.0f),
            Vector3f(0.0f, 0.0f, -1.0f),
            Vector3f(0.0f, 0.0f, -1.0f),
            Vector3f(0.0f, 0.0f, -1.0f),
            //+x
            Vector3f(1.0f, 0.0f, 0.0f),
            Vector3f(1.0f, 0.0f, 0.0f),
            Vector3f(1.0f, 0.0f, 0.0f),
            Vector3f(1.0f, 0.0f, 0.0f),
            //-x
            Vector3f(-1.0f, 0.0f, 0.0f),
            Vector3f(-1.0f, 0.0f, 0.0f),
            Vector3f(-1.0f, 0.0f, 0.0f),
            Vector3f(-1.0f, 0.0f, 0.0f),
            //+y
            Vector3f(0.0f, 1.0f, 0.0f),
            Vector3f(0.0f, 1.0f, 0.0f),
            Vector3f(0.0f, 1.0f, 0.0f),
            Vector3f(0.0f, 1.0f, 0.0f),
            //-y
            Vector3f(0.0f, -1.0f, 0.0f),
            Vector3f(0.0f, -1.0f, 0.0f),
            Vector3f(0.0f, -1.0f, 0.0f),
            Vector3f(0.0f, -1.0f, 0.0f),
        };

    std::vector<Vector2f> texcoords =
        {
            //+z
            Vector2f(0.0f, 1.0f),
            Vector2f(0.0f, 0.0f),
            Vector2f(1.0f, 0.0f),
            Vector2f(1.0f, 1.0f),
            //-z
            Vector2f(0.0f, 1.0f),
            Vector2f(0.0f, 0.0f),
            Vector2f(1.0f, 0.0f),
            Vector2f(1.0f, 1.0f),
            //+x
            Vector2f(0.0f, 1.0f),
            Vector2f(0.0f, 0.0f),
            Vector2f(1.0f, 0.0f),
            Vector2f(1.0f, 1.0f),
            //-x
            Vector2f(0.0f, 1.0f),
            Vector2f(0.0f, 0.0f),
            Vector2f(1.0f, 0.0f),
            Vector2f(1.0f, 1.0f),
            //+y
            Vector2f(0.0f, 1.0f),
            Vector2f(0.0f, 0.0f),
            Vector2f(1.0f, 0.0f),
            Vector2f(1.0f, 1.0f),
            //-y
            Vector2f(0.0f, 1.0f),
            Vector2f(0.0f, 0.0f),
            Vector2f(1.0f, 0.0f),
            Vector2f(1.0f, 1.0f),
        };

    std::vector<Vector3f> tangents =
        {
            //+z
            Vector3f(1.0f, 0.0f, 0.0f),
            Vector3f(1.0f, 0.0f, 0.0f),
            Vector3f(1.0f, 0.0f, 0.0f),
            Vector3f(1.0f, 0.0f, 0.0f),
            //-z
            Vector3f(-1.0f, 0.0f, 0.0f),
            Vector3f(-1.0f, 0.0f, 0.0f),
            Vector3f(-1.0f, 0.0f, 0.0f),
            Vector3f(-1.0f, 0.0f, 0.0f),
            //+x
            Vector3f(0.0f, 0.0f, -1.0f),
            Vector3f(0.0f, 0.0f, -1.0f),
            Vector3f(0.0f, 0.0f, -1.0f),
            Vector3f(0.0f, 0.0f, -1.0f),
            //-x
            Vector3f(0.0f, 0.0f, 1.0f),
            Vector3f(0.0f, 0.0f, 1.0f),
            Vector3f(0.0f, 0.0f, 1.0f),
            Vector3f(0.0f, 0.0f, 1.0f),
            //+y
            Vector3f(1.0f, 0.0f, 0.0f),
            Vector3f(1.0f, 0.0f, 0.0f),
            Vector3f(1.0f, 0.0f, 0.0f),
            Vector3f(1.0f, 0.0f, 0.0f),
            //-y
            Vector3f(1.0f, 0.0f, 0.0f),
            Vector3f(1.0f, 0.0f, 0.0f),
            Vector3f(1.0f, 0.0f, 0.0f),
            Vector3f(1.0f, 0.0f, 0.0f),
        };

    std::vector<uint32_t> indices =
        {
            0,
            1,
            2,
            0,
            2,
            3,

            4,
            5,
            6,
            4,
            6,
            7,

            8,
            9,
            10,
            8,
            10,
            11,

            12,
            13,
            14,
            12,
            14,
            15,

            16,
            17,
            18,
            16,
            18,
            19,

            20,
            21,
            22,
            20,
            22,
            23,
        };

    PbrMesh mesh;
    for (int32_t i = 0; i < 24; ++i)
    {
        PbrVertex v;
        v.position = positions[i];
        v.normal = normals[i];
        v.texcoord = texcoords[i];
        mesh.mVertices.emplace_back(v);
    }

    mesh.mIndices = indices;

    PbrMeshInstance instance;
    instance.mesh = mesh;

    mMeshInstances.emplace_back(instance);
}
void PbrModel::CreateBuiltInSphere()
{
    std::vector<Vector3f> positions;
    std::vector<Vector3f> normals;
    std::vector<Vector2f> texcoords;
    std::vector<Vector3f> tangents;
    std::vector<uint32_t> indices;
    float latitudeBands = 20.0f;
    float longitudeBands = 20.0f;
    float radius = 1.0f;

    for (float latNumber = 0; latNumber <= latitudeBands; latNumber++)
    {
        float theta = latNumber * 3.14 / latitudeBands;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);

        for (float longNumber = 0; longNumber <= longitudeBands; longNumber++)
        {
            float phi = longNumber * 2 * 3.147 / longitudeBands;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);

            Vector3f tmp = Vector3f(cosPhi * sinTheta, cosTheta, sinPhi * sinTheta);

            positions.emplace_back(tmp * radius);
            normals.emplace_back(tmp);
            texcoords.emplace_back(Vector2f((longNumber / longitudeBands), (latNumber / latitudeBands)));

            Vector3f tangent = Vector3f(cosPhi, 0.0f, -sinPhi);

            tangents.emplace_back(tangent);
        }
    }

    for (uint32_t latNumber = 0; latNumber < latitudeBands; latNumber++)
    {
        for (uint32_t longNumber = 0; longNumber < longitudeBands; longNumber++)
        {
            uint32_t first = (latNumber * (longitudeBands + 1)) + longNumber;
            uint32_t second = first + longitudeBands + 1;

            indices.emplace_back(first);
            indices.emplace_back(second);
            indices.emplace_back(first + 1);

            indices.emplace_back(second);
            indices.emplace_back(second + 1);
            indices.emplace_back(first + 1);
        }
    }

    PbrMesh mesh;
    for (int32_t i = 0; i < 24; ++i)
    {
        PbrVertex v;
        v.position = positions[i];
        v.normal = normals[i];
        v.texcoord = texcoords[i];
        mesh.mVertices.emplace_back(v);
    }

    mesh.mIndices = indices;

    PbrMeshInstance instance;
    instance.mesh = mesh;

    mMeshInstances.emplace_back(instance);
}

const std::vector<PbrMeshInstance> &PbrModel::GetMeshInstances() const
{
    return mMeshInstances;
}

const Matrix4f &PbrModel::GetModelMat()
{
    return mModelMat;
}