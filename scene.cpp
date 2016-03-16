#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <scene.hpp>
#include <streamops.hpp>

std::string dirname(const std::string& path) {
    auto index = path.find_last_of("/\\");

    if (index != std::string::npos) {
        while (index != 0 && (path[index - 1] == '\\' || path[index - 1] == '/')) {
            --index;
        }

        return path.substr(0, index);
    }
    else {
        return path;
    }
}

namespace haste {

string name(const aiMaterial* material) {
    aiString name;
    material->Get(AI_MATKEY_NAME, name);
    return name.C_Str();
}

vec3 ambient(const aiMaterial* material) {
    aiColor3D result;
    material->Get(AI_MATKEY_COLOR_AMBIENT, result);
    return vec3(result.r, result.g, result.b);
}

vec3 diffuse(const aiMaterial* material) {
    aiColor3D result;
    material->Get(AI_MATKEY_COLOR_DIFFUSE, result);
    return vec3(result.r, result.g, result.b);
}

vec3 emissive(const aiMaterial* material) {
    aiColor3D result;
    material->Get(AI_MATKEY_COLOR_EMISSIVE, result);
    return vec3(result.r, result.g, result.b);
}

vec3 specular(const aiMaterial* material) {
    aiColor3D result;
    material->Get(AI_MATKEY_COLOR_SPECULAR, result);
    return vec3(result.r, result.g, result.b);
}

vec3 toVec3(const aiVector3D& v) {
    return vec3(v.x, v.y, v.z);
}

bool isMeshALight(const aiScene* scene, size_t meshID) {
    size_t materialID = scene->mMeshes[meshID]->mMaterialIndex;
    auto material = scene->mMaterials[materialID];
    return emissive(material) != vec3(0.0f);
}

Light meshToLight(const aiScene* scene, size_t meshID) {
    auto mesh = scene->mMeshes[meshID];
    size_t materialID = mesh->mMaterialIndex;
    auto material = scene->mMaterials[materialID];

    Light result;
    result.name = name(material);
    result.type = LightType::Area;
    result.emissive = emissive(material);

    const string error = "Cannot convert \"" + result.name + "\" mesh to light.";

    if (mesh->mNumFaces != 2 ||
        mesh->mFaces[0].mNumIndices != 3 ||
        mesh->mFaces[1].mNumIndices != 3 ||
        mesh->mNumVertices != 4) {
        throw std::runtime_error(error);
    }

    unsigned common[2];
    size_t itr = 0;

    for (size_t i = 0; i < mesh->mFaces[0].mNumIndices; ++i) {
        for (size_t j = 0; j < mesh->mFaces[1].mNumIndices; ++j) {
            if (mesh->mFaces[0].mIndices[i] == mesh->mFaces[1].mIndices[j]) {
                common[itr] = mesh->mFaces[0].mIndices[i];
                ++itr;
            }
        }
    }

    if (itr != 2) {
        throw std::runtime_error(error);
    }    

    bool originSet = false;
    unsigned origin;

    for (size_t i = 0; i < mesh->mFaces[0].mNumIndices; ++i) {
        size_t j = 0;

        while (j < itr) {
            if (mesh->mFaces[0].mIndices[i] == common[j]) {
                break;
            }

            ++j;
        }

        if (j == itr) {
            originSet = true;
            origin = mesh->mFaces[0].mIndices[i];
        }
    }

    if (!originSet) {
        throw std::runtime_error(error);
    }

    vec3 v0 = toVec3(mesh->mVertices[origin]);
    vec3 v1 = toVec3(mesh->mVertices[common[0]]);
    vec3 v2 = toVec3(mesh->mVertices[common[1]]);

    result.area.u = v1 - v0;
    result.area.v = v2 - v0;
    result.area.position = v0 + (result.area.u + result.area.v) * 0.5f - vec3(0, 0.1, 0);

    return result;
}

Scene loadScene(RTCDevice device, string path) {
    Assimp::Importer importer;

    auto flags =
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices;

    const aiScene* scene = importer.ReadFile(path, flags);

    if (!scene) {
        throw std::runtime_error("Cannot load \"" + path + "\" scene.");
    }

    RTCScene rtcScene = rtcDeviceNewScene(device, RTC_SCENE_STATIC, RTC_INTERSECT1);

    if (rtcScene == nullptr) {
        throw std::runtime_error("Cannot create RTCScene.");
    }

    vector<Mesh> meshes;
    vector<Light> lights;

    for (size_t i = 0; i < scene->mNumMeshes; ++i) {
        if (isMeshALight(scene, i)) {
            lights.push_back(meshToLight(scene, i));
        }
        
        unsigned geomID = rtcNewTriangleMesh(
            rtcScene,
            RTC_GEOMETRY_STATIC,
            scene->mMeshes[i]->mNumFaces,
            scene->mMeshes[i]->mNumVertices,
            1);

        vec4* vbuffer = (vec4*) rtcMapBuffer(rtcScene, geomID, RTC_VERTEX_BUFFER);

        for (size_t j = 0; j < scene->mMeshes[i]->mNumVertices; ++j) {
            vbuffer[j].x = scene->mMeshes[i]->mVertices[j].x;
            vbuffer[j].y = scene->mMeshes[i]->mVertices[j].y;
            vbuffer[j].z = scene->mMeshes[i]->mVertices[j].z;
            vbuffer[j].w = 1;
        }

        rtcUnmapBuffer(rtcScene, geomID, RTC_VERTEX_BUFFER);

        int* triangles = (int*) rtcMapBuffer(rtcScene, geomID, RTC_INDEX_BUFFER);

        vector<int> indices(scene->mMeshes[i]->mNumFaces * 3);

        for (size_t j = 0; j < scene->mMeshes[i]->mNumFaces; ++j) {
            if (scene->mMeshes[i]->mFaces[j].mNumIndices != 3) {
                rtcUnmapBuffer(rtcScene, geomID, RTC_INDEX_BUFFER);
                rtcDeleteScene(rtcScene);
                throw std::runtime_error("Loaded scene contains non triangle faces.");
            }

            for (size_t k = 0; k < 3; ++k) {
                triangles[j * 3 + k] = scene->mMeshes[i]->mFaces[j].mIndices[k];
                indices[j * 3 + k] = scene->mMeshes[i]->mFaces[j].mIndices[k];
            }
        }

        rtcUnmapBuffer(rtcScene, geomID, RTC_INDEX_BUFFER);

        if (geomID != meshes.size()) {
            rtcDeleteScene(rtcScene);
            throw std::runtime_error("Geometry ID doesn't correspond to mesh index.");
        }

        if (scene->mMeshes[i]->mNormals == nullptr) {
            rtcDeleteScene(rtcScene);
            throw std::runtime_error("Normal vectors are not present.");
        }

        vector<vec3> normals(scene->mMeshes[i]->mNumVertices);

        for (size_t j = 0; j < scene->mMeshes[i]->mNumVertices; ++j) {
            normals[j] = toVec3(scene->mMeshes[i]->mNormals[j]);
        }

        Mesh mesh;
        mesh.materialID = scene->mMeshes[i]->mMaterialIndex;
        mesh.name = scene->mMeshes[i]->mName.C_Str();
        mesh.indices = std::move(indices);
        mesh.normals = std::move(normals);
        meshes.push_back(mesh);
    }

    rtcCommit(rtcScene);

    vector<Material> materials;

    for (size_t i = 0; i < scene->mNumMaterials; ++i) {
        Material material;
        material.name = name(scene->mMaterials[i]);
        material.ambient = ambient(scene->mMaterials[i]);
        material.diffuse = diffuse(scene->mMaterials[i]);
        
        materials.push_back(material);
    }

    Scene result;
    result.rtcScene = rtcScene;
    result.lights = std::move(lights);
    result.meshes = std::move(meshes);
    result.materials = std::move(materials);
    return result;
}

}
