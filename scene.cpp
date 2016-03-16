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

bool isEmissive(const aiScene* scene, size_t meshID) {
    size_t materialID = scene->mMeshes[meshID]->mMaterialIndex;
    auto material = scene->mMaterials[materialID];
    return emissive(material) != vec3(0.0f);
}

Mesh makeMesh(
    RTCScene& rtcScene,
    const aiScene* scene,
    size_t i,
    size_t numMeshes) {
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

    if (geomID != numMeshes) {
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
    return mesh;
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
    vector<size_t> emissive;

    for (size_t i = 0; i < scene->mNumMeshes; ++i) {
        if (isEmissive(scene, i)) {
            emissive.push_back(i);
        }
        else {
            Mesh mesh = makeMesh(
                rtcScene,
                scene,
                i,
                meshes.size());

            meshes.push_back(mesh);
        }
    }

    vector<Mesh> areaLights;

    for (size_t i = 0; i < emissive.size(); ++i) {
        Mesh mesh = makeMesh(
            rtcScene,
            scene,
            emissive[i],
            meshes.size() + areaLights.size());

        areaLights.push_back(mesh);
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
    result.meshes = std::move(meshes);
    result.areaLights = std::move(areaLights);
    result.materials = std::move(materials);
    return result;
}

unsigned makeRTCMesh(RTCScene rtcScene, size_t i, const vector<Mesh>& meshes) {
    unsigned geomID = rtcNewTriangleMesh(
        rtcScene,
        RTC_GEOMETRY_STATIC,
        meshes[i].indices.size() / 3,
        meshes[i].vertices.size(),
        1);

    vec4* vbuffer = (vec4*) rtcMapBuffer(rtcScene, geomID, RTC_VERTEX_BUFFER);

    for (size_t j = 0; j < meshes[i].vertices.size(); ++j) {
        vbuffer[j].x = meshes[i].vertices[j].x;
        vbuffer[j].y = meshes[i].vertices[j].y;
        vbuffer[j].z = meshes[i].vertices[j].z;
        vbuffer[j].w = 1;
    }

    rtcUnmapBuffer(rtcScene, geomID, RTC_VERTEX_BUFFER);

    int* triangles = (int*) rtcMapBuffer(rtcScene, geomID, RTC_INDEX_BUFFER);
    std::memcpy(
        triangles,
        meshes[i].indices.data(),
        meshes[i].indices.size() * sizeof(int));

    rtcUnmapBuffer(rtcScene, geomID, RTC_INDEX_BUFFER);

    return geomID;
}

void updateCache(Cache& cache, RTCDevice device, const Scene& scene) {
    RTCScene rtcScene = rtcDeviceNewScene(device, RTC_SCENE_STATIC, RTC_INTERSECT1);

    if (rtcScene == nullptr) {
        throw std::runtime_error("Cannot create RTCScene.");
    }

    for (size_t i = 0; i < scene.meshes.size(); ++i) {
        unsigned geomID = makeRTCMesh(rtcScene, i, scene.meshes);

        if (geomID != i) {
            rtcDeleteScene(rtcScene);
            throw std::runtime_error("Geometry ID doesn't correspond to mesh index.");
        }
    }

    for (size_t i = 0; i < scene.areaLights.size(); ++i) {
        unsigned geomID = makeRTCMesh(rtcScene, i, scene.areaLights);

        if (geomID != scene.meshes.size() + i) {
            rtcDeleteScene(rtcScene);
            throw std::runtime_error("Geometry ID doesn't correspond to mesh index.");
        }
    }

    rtcCommit(rtcScene);

    if (cache.rtcScene != nullptr) {
        rtcDeleteScene(cache.rtcScene);
    }

    cache.rtcScene = rtcScene;
}

}
