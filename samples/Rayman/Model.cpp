#include "Model.h"
#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>
#include <iostream>
#include "Math/Vector4.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Matrix4.h"
#include "Math/Quaternion.h"

std::vector<float> GetScalarValues(uint32_t compCount, const cgltf_accessor &inAccessor)
{
    std::vector<float> out;
    out.resize(inAccessor.count * compCount);
    for (cgltf_size i = 0; i < inAccessor.count; ++i)
        cgltf_accessor_read_float(&inAccessor, i, &out[i * compCount], compCount);
    return out;
}

void ModelDef::LoadFromFile(const std::string &path)
{
    cgltf_options options;
    memset(&options, 0, sizeof(cgltf_options));
    cgltf_data *data = nullptr;
    cgltf_result result = cgltf_parse_file(&options, path.c_str(), &data);
    if (result != cgltf_result_success)
    {
        std::cout << "Failed to load file:" << path << std::endl;
        return;
    }
    result = cgltf_load_buffers(&options, data, path.c_str());
    if (result != cgltf_result_success)
    {
        cgltf_free(data);
        std::cout << "Failed to load file:" << path << std::endl;
        return;
    }
    result = cgltf_validate(data);
    if (result != cgltf_result_success)
    {
        cgltf_free(data);
        std::cout << "Failed to load file:" << path << std::endl;
        return;
    }
    cgltf_node *nodes = data->nodes;
    uint32_t nodeCount = data->nodes_count;
    for (uint32_t i = 0; i < nodeCount; ++i)
    {
        cgltf_node *node = &nodes[i];
        if (node->mesh == 0)
            continue;

        Matrix4f localTransform=Matrix4f::IDENTITY;

        Matrix4f trans = Matrix4f::Translate(Vector3f(node->translation[0], node->translation[1], node->translation[2]));
        Matrix4f rot = Quaternionf::ToMatrix4(Quaternionf(node->rotation[0], node->rotation[1], node->rotation[2], node->rotation[3]));
        Matrix4f scale = Matrix4f::Scale(Vector3f(node->scale[0], node->scale[1], node->scale[2]));

        localTransform = trans * rot * scale;

        cgltf_node *parent = node->parent;
        while (parent)
        {
            trans = Matrix4f::Translate(Vector3f(parent->translation[0], parent->translation[1], parent->translation[2]));
            rot = Quaternionf::ToMatrix4(Quaternionf(parent->rotation[0], parent->rotation[1], parent->rotation[2], parent->rotation[3]));
            scale = Matrix4f::Scale(Vector3f(parent->scale[0], parent->scale[1], parent->scale[2]));
            Matrix4f parentLocalTransform = trans * rot * scale;
            localTransform = parentLocalTransform * localTransform;
            parent = parent->parent;
        }

        int numPrims = node->mesh->primitives_count;
        for (int j = 0; j < numPrims; ++j)
        {
            cgltf_primitive *primitive = &node->mesh->primitives[j];
            uint32_t ac = primitive->attributes_count;
            std::vector<Vector3f> positions;
            std::vector<Vector3f> normals;
            std::vector<Vector2f> texcoords;
            std::vector<uint32_t> indices;
            for (uint32_t k = 0; k < ac; ++k)
            {
                cgltf_attribute *attribute = &primitive->attributes[k];
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
                    {
                        Vector3f p = Vector3f(values[index + 0], values[index + 1], values[index + 2]);
                        p = Vector3f(localTransform * Vector4f(p, 1.0));
                        positions.emplace_back(p);
                        break;
                    }
                    case cgltf_attribute_type_texcoord:
                        texcoords.emplace_back(Vector2f(values[index + 0], values[index + 1]));
                        break;
                    case cgltf_attribute_type_normal:
                    {
                        Vector3f n = Vector3f(values[index + 0], values[index + 1], values[index + 2]);
                        n = Vector3f(Matrix4f::Transpose(Matrix4f::Inverse(localTransform)) * Vector4f(n, 1.0f));
                        normals.emplace_back(n);
                        break;
                    }
                    default:
                        break;
                    }
                }
            }
            if (primitive->indices != 0)
            {
                for (uint32_t k = 0; k < primitive->indices->count; ++k)
                    indices.emplace_back(cgltf_accessor_read_index(primitive->indices, k));
            }
            std::vector<Vertex> vertices;
            for (size_t i = 0; i < positions.size(); ++i)
            {
                Vertex v;
                v.position = positions[i];
                v.normal = normals[i];
                if (!texcoords.empty())
                    v.texcoord = texcoords[i];
                vertices.emplace_back(v);
            }
            meshes.emplace_back(new MeshDef(vertices, indices));
        }
    }

    cgltf_free(data);
}