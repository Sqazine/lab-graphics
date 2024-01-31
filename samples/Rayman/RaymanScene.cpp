#include "RaymanScene.h"

#include <cstdio>
#include <iostream>
#include <sstream>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <map>
#include <rapidjson/document.h>
#include <utility>
#include <stb/stb_image.h>
#include "Texture.h"
#include "App.h"
#include "Model.h"
#include "InputSystem.h"
#include "Math/Quaternion.h"
#include "Math/Vector4.h"
#include "RtxRayTraceScene.h"
#include "RtxRayTracePass.h"

std::string ExtractFileDirectory(std::string_view path)
{
    return std::string(path).substr(0, path.find_last_of('/') + 1);
}

ImageData *Load2DImageData(const std::string &path)
{
    static std::unordered_map<std::string, ImageData *> textureDataPool;
    auto iter = textureDataPool.find(path);
    if (iter != textureDataPool.end())
        return iter->second;
    else
    {
        int texWidth;
        int texHeight;
        int channels;
        uint8_t *pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &channels, STBI_rgb_alpha);

        if (!pixels)
        {
            std::cout << "[ERROR] Failed to load texture image:" << path << std::endl;
            exit(1);
        }
        auto newImageData = new ImageData(ImageDataType::RGBA8, texWidth, texHeight, 4, pixels);
        textureDataPool[path] = newImageData;
        return newImageData;
    }
}

static Matrix4f GetEntityNodeSRTMat(const rapidjson::Value &entity)
{
    Vector3f position;
    position.x = entity["position"][0].GetFloat();
    position.y = entity["position"][1].GetFloat();
    position.z = entity["position"][2].GetFloat();

    Vector3f rotation;
    rotation.x = entity["rotation"][0].GetFloat();
    rotation.y = entity["rotation"][1].GetFloat();
    rotation.z = entity["rotation"][2].GetFloat();

    Vector3f scale;
    scale.x = entity["scale"][0].GetFloat();
    scale.y = entity["scale"][1].GetFloat();
    scale.z = entity["scale"][2].GetFloat();

    Matrix4f xform = Matrix4f::Translate(position) * Quaternionf::ToMatrix4(Quaternionf(rotation)) * Matrix4f::Scale(scale);

    return xform;
}

RaymanScene::RaymanScene(std::string_view filePath)
{
    LoadFromFile(filePath);
}

void RaymanScene::AddHDR(HDRData *hdr)
{
    hdrResolution = (float)hdr->width * hdr->height;

    hdrColumns = std::make_unique<ImageData>(ImageDataType::HDR, hdr->width, hdr->height, 12, hdr->cols);

    hdrConditional = std::make_unique<ImageData>(ImageDataType::HDR, hdr->width, hdr->height, 12, hdr->conditionalDistData);

    hdrMarginal = std::make_unique<ImageData>(ImageDataType::HDR, hdr->width, hdr->height, 12, hdr->marginalDistData);
}

int RaymanScene::AddTexture(ImageData *texture)
{
    int id = 0;

    bool exists = false;
    for (int i = 0; i < textureDatas.size(); ++i)
        if (textureDatas[i].get() == texture)
        {
            id = i;
            exists = true;
            break;
        }

    if (!exists)
    {
        id = (int)textureDatas.size();
        textureDatas.emplace_back(texture);
    }

    return id;
}

RaymanScene::~RaymanScene()
{
}

void RaymanScene::LoadFromFile(std::string_view filePath)
{
    std::ifstream file;
    file.open(filePath.data());
    assert(file.is_open());

    std::stringstream sstr;
    sstr << file.rdbuf();
    file.close();

    auto sceneJsonDir = ExtractFileDirectory(filePath);

    std::string jsonContent = sstr.str();
    rapidjson::StringStream jsonStr(jsonContent.c_str());
    rapidjson::Document doc;
    doc.ParseStream(jsonStr);
    if (!doc.IsObject())
    {
        std::cout << "[ERROR] Failed to read scene file:" << filePath << std::endl;
        exit(1);
    }

    const rapidjson::Value &lights = doc["Lights"];
    const rapidjson::Value &entities = doc["Entities"];
    const rapidjson::Value &materials = doc["Material"];

    //======================================load light================================
    for (rapidjson::SizeType i = 0; i < lights.Size(); ++i)
    {
        Light light;
        Vector3f v1;
        Vector3f v2;
        std::string lightType = "none";

        light.position.x = lights[i]["position"][0].GetFloat();
        light.position.y = lights[i]["position"][1].GetFloat();
        light.position.z = lights[i]["position"][2].GetFloat();

        light.emission.x = lights[i]["emission"][0].GetFloat();
        light.emission.y = lights[i]["emission"][1].GetFloat();
        light.emission.z = lights[i]["emission"][2].GetFloat();

        v1.x = lights[i]["v1"][0].GetFloat();
        v1.y = lights[i]["v1"][1].GetFloat();
        v1.z = lights[i]["v1"][2].GetFloat();

        v2.x = lights[i]["v2"][0].GetFloat();
        v2.y = lights[i]["v2"][1].GetFloat();
        v2.z = lights[i]["v2"][2].GetFloat();

        lightType = lights[i]["type"].GetString();

        if (lightType == "quad")
        {
            light.type = LightType::QuadLight;
            light.u = v1 - light.position;
            light.v = v2 - light.position;
            light.area = Vector3f::Cross(Vector4f::ToVector3(light.u), Vector4f::ToVector3(light.v)).Length();
        }
        else if (lightType == "sphere")
        {
            light.type = LightType::SphereLight;
            light.area = 4.0f * Math::PI * light.radius * light.radius;
        }

        AddLight(light);
    }

    struct MaterialData
    {
        Material mat;
        int id;
    };

    std::map<std::string, MaterialData> materialMap;

    //======================================load material================================
    for (rapidjson::SizeType i = 0; i < materials.Size(); ++i)
    {

        std::string matName = materials[i]["name"].GetString();

        Material material{};

        const rapidjson::Value &matData = materials[i];

        // albedo color
        if (matData.HasMember("albedo") && matData["albedo"].IsArray())
        {
            material.albedoColor.x = pow(matData["albedo"][0].GetFloat(), 2.2);
            material.albedoColor.y = pow(matData["albedo"][1].GetFloat(), 2.2);
            material.albedoColor.z = pow(matData["albedo"][2].GetFloat(), 2.2);
        }

        // specular color
        if (matData.HasMember("specular") && matData["specular"].IsArray())
        {
            material.specularColor.x = matData["specular"][0].GetFloat();
            material.specularColor.y = matData["specular"][1].GetFloat();
            material.specularColor.z = matData["specular"][2].GetFloat();
        }

        // emission color
        if (matData.HasMember("emission") && matData["emission"].IsArray())
        {
            material.emissionColor.x = matData["emission"][0].GetFloat();
            material.emissionColor.y = matData["emission"][1].GetFloat();
            material.emissionColor.z = matData["emission"][2].GetFloat();
        }

        // emission intensity
        if (matData.HasMember("emissionIntensity"))
            material.emissionIntensity = matData["emissionIntensity"].GetFloat();

        // extinction color
        if (matData.HasMember("attenuationColor") && matData["attenuationColor"].IsArray())
        {
            material.extinctionColor.x = matData["attenuationColor"][0].GetFloat();
            material.extinctionColor.y = matData["attenuationColor"][1].GetFloat();
            material.extinctionColor.z = matData["attenuationColor"][2].GetFloat();
        }

        // roughness
        if (matData.HasMember("roughness") && matData["roughness"].IsNumber())
            material.roughness = matData["roughness"].GetFloat();

        if (matData.HasMember("metallic") && matData["metallic"].IsNumber())
            material.metallic = matData["metallic"].GetFloat();
        else
            material.metallic = 1.0f;

        // attenuationDistance
        if (matData.HasMember("attenuationDistance") && matData["attenuationDistance"].IsNumber())
            material.attenuationDistance = matData["attenuationDistance"].GetFloat();

        // ior
        if (matData.HasMember("ior") && matData["ior"].IsNumber())
            material.ior = matData["ior"].GetFloat();

        // transmission
        if (matData.HasMember("transmission") && matData["transmission"].IsNumber())
            material.transmission = matData["transmission"].GetFloat();

        // thickness
        if (matData.HasMember("thickness") && matData["thickness"].IsNumber())
            material.thickness = matData["thickness"].GetFloat();

        // bumpiness
        if (matData.HasMember("bumpiness") && matData["bumpiness"].IsNumber())
            material.bumpiness = matData["bumpiness"].GetFloat();

        // clearcoat
        if (matData.HasMember("clearCoat") && matData["clearCoat"].IsNumber())
            material.clearcoat = matData["clearCoat"].GetFloat();

        // clearcoat glossiness
        if (matData.HasMember("clearCoatGlossiness") && matData["clearCoatGlossiness"].IsNumber())
            material.clearcoatGloss = matData["clearCoatGlossiness"].GetFloat();

        // albedo map
        if (matData.HasMember("albedoMap") && matData["albedoMap"].IsString())
        {
            std::string filePath = matData["albedoMap"].GetString();
            material.albedoTexID = AddTexture(Load2DImageData(sceneJsonDir + filePath));
        }

        // normal map
        if (matData.HasMember("normalMap") && matData["normalMap"].IsString())
        {
            std::string filePath = matData["normalMap"].GetString();
            material.normalTexID = AddTexture(Load2DImageData(sceneJsonDir + filePath));
        }

        // metallic map
        if (matData.HasMember("metallicmap") && matData["metallicmap"].IsString())
        {
            std::string filePath = matData["metallicmap"].GetString();
            material.metallicTexID = AddTexture(Load2DImageData(sceneJsonDir + filePath));
        }

        // roughness map
        if (matData.HasMember("roughnessMap") && matData["roughnessMap"].IsString())
        {
            std::string filePath = matData["roughnessMap"].GetString();
            material.roughnessTexID = AddTexture(Load2DImageData(sceneJsonDir + filePath));
        }

        // emission map
        if (matData.HasMember("emissionMap") && matData["emissionMap"].IsString())
        {
            std::string filePath = matData["emissionMap"].GetString();
            material.emissionTexID = AddTexture(Load2DImageData(sceneJsonDir + filePath));
        }

        // opacity map
        if (matData.HasMember("opacityMap") && matData["opacityMap"].IsString())
        {
            std::string filePath = matData["opacityMap"].GetString();
            material.opacityTexID = AddTexture(Load2DImageData(sceneJsonDir + filePath));
        }

        if (materialMap.find(matName) == materialMap.end()) // New material
        {
            int id = AddMaterial(material);
            materialMap[matName] = MaterialData{material, id};
        }

        std::cout << "loaded Material:" << matName << std::endl;
    }

    //====================================load camera==============================
    Vector3f target{};
    target.x = entities[0]["target"][0].GetFloat();
    target.y = entities[0]["target"][1].GetFloat();
    target.z = entities[0]["target"][2].GetFloat();

    Vector3f position{};
    position.x = entities[0]["position"][0].GetFloat();
    position.y = entities[0]["position"][1].GetFloat();
    position.z = entities[0]["position"][2].GetFloat();

    float fov = entities[0]["fov"].GetFloat();

    float aspect = entities[0]["aspect"].GetFloat();

    SetCamera(position, target, fov, aspect);

    //==========================================load scene hdr=========================================================
    const rapidjson::Value &sceneHdr = entities[1];
    if (!sceneHdr["resource"].IsNull())
    {
        auto sceneHdrFilePath = sceneHdr["resource"].GetString();
        auto hdrName = sceneJsonDir + sceneHdrFilePath;
        auto *hdr = LoadHDR(hdrName.c_str());

        if (hdr == nullptr)
        {
            std::cout << "[ERROR] unable to load HDR:" << hdrName << std::endl;
            exit(1);
        }
        else
            std::cout << "loaded scene hdr:" << hdrName << std::endl;

        AddHDR(hdr);
    }

    //==========================================load model entity======================================================
    std::unordered_map<std::string, ModelDef> models;
    for (rapidjson::SizeType i = 2; i < entities.Size(); ++i)
    {
        const rapidjson::Value &entity = entities[i];

        std::string modelFilePath = entity["resource"].GetString();

        ModelDef targetModel;
        auto iter = models.find(modelFilePath);
        if (iter != models.end())
            targetModel = iter->second;
        else
        {
            targetModel.LoadFromFile(sceneJsonDir + modelFilePath);
            models[modelFilePath] = targetModel;
            std::cout << "loaded model:" << modelFilePath << std::endl;
        }

        Matrix4f worldMat = GetEntityNodeSRTMat(entity);

        if (entity.HasMember("materials"))
        {
            ModelDef &modelRef = targetModel;
            const rapidjson::Value &materialsNode = entity["materials"];
            for (auto idx = 0; idx != materialsNode.GetArray().Size(); ++idx)
                modelRef.matNames.emplace_back(materialsNode[idx].GetString());
        }

        for (size_t j = 0; j < targetModel.meshes.size(); ++j)
        {
            int mesh_id = AddMesh(targetModel.meshes[j]);
            if (mesh_id != -1)
            {
                MeshInstance instance(mesh_id, worldMat, materialMap[targetModel.matNames[j]].id);
                AddMeshInstance(instance);
            }
        }
    }
}

void RaymanScene::SetCamera(Vector3f position, Vector3f target, float fov, float aspect)
{
    camera.reset(new RmCamera(position, target, fov, aspect));
}

int RaymanScene::AddMeshInstance(MeshInstance meshInstance)
{
    int id = (int)meshInstances.size();
    meshInstances.emplace_back(meshInstance);
    return id;
}

int RaymanScene::AddMesh(MeshDef *mesh)
{
    int id;

    MeshDef *new_mesh = mesh->Clone();
    id = (int)meshes.size();
    meshes.emplace_back(new_mesh);

    return id;
}

int RaymanScene::AddMaterial(Material material)
{
    int id = (int)materials.size();
    materials.emplace_back(material);
    return id;
}

int RaymanScene::AddLight(Light light)
{
    int id = (int)lights.size();
    lights.emplace_back(light);
    return id;
}

void RaymanScene::Init()
{
    // App::Instance().GetWindow()->Resize(1600, 900);

    mRtxRayTraceScene = std::make_unique<RtxRayTraceScene>(this);
}

void RaymanScene::ProcessInput()
{
    if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_UP) == ButtonState::PRESS)
    {
        hdrMultiplier++;
        mRtxRayTraceScene->ResetAccumulation();
    }
    if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_DOWN) == ButtonState::PRESS)
    {
        hdrMultiplier--;
        hdrMultiplier = std::max(0.0f, hdrMultiplier);
        mRtxRayTraceScene->ResetAccumulation();
    }

    if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_O) == ButtonState::PRESS)
        mRtxRayTraceScene->SaveOutputImageToDisk();

    if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_1) == ButtonState::PRESS)
        mRtxRayTraceScene->SetRenderState(RenderState::FINAL);

    if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_2) == ButtonState::PRESS)
        mRtxRayTraceScene->SetRenderState(RenderState::AO);

    if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_3) == ButtonState::PRESS)
        mRtxRayTraceScene->SetRenderState(RenderState::ALBEDO);

    if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_4) == ButtonState::PRESS)
        mRtxRayTraceScene->SetRenderState(RenderState::NORMAL);

    if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_P) == ButtonState::PRESS)
    {
        if (App::Instance().GetInputSystem().GetMouse().IsReleativeMode())
            App::Instance().GetInputSystem().GetMouse().SetReleativeMode(false);
        else
            App::Instance().GetInputSystem().GetMouse().SetReleativeMode(true);
    }

    static int32_t counter = 0;
    if (App::Instance().GetInputSystem().GetKeyboard().GetKeyState(SDL_SCANCODE_SPACE) == ButtonState::PRESS)
    {
        if (counter % 4 == 0)
            mRtxRayTraceScene->SetPostProcessType(PostProcessType::NONE);
        else if (counter % 4 == 1)
            mRtxRayTraceScene->SetPostProcessType(PostProcessType::DENOISER);
        else if (counter % 4 == 2)
            mRtxRayTraceScene->SetPostProcessType(PostProcessType::EDGE_DETECTER);
        else if (counter % 4 == 3)
            mRtxRayTraceScene->SetPostProcessType(PostProcessType::SHARPENER);
        counter++;
    }

    camera->ProcessInput();
}

void RaymanScene::Update()
{
    mRtxRayTraceScene->Update();

    camera->Update();
}

void RaymanScene::Render()
{
    mRtxRayTraceScene->Render();
}