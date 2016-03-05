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

Scene loadScene(RTCDevice device, string path) {
    Assimp::Importer importer;

    auto flags =
        aiProcess_Triangulate |
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

    for (size_t i = 0; i < scene->mNumMeshes; ++i) {
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

        for (size_t j = 0; j < scene->mMeshes[i]->mNumFaces; ++j) {
            if (scene->mMeshes[i]->mFaces[j].mNumIndices != 3) {
                rtcUnmapBuffer(rtcScene, geomID, RTC_INDEX_BUFFER);
                rtcDeleteScene(rtcScene);
                throw std::runtime_error("Loaded scene contains non triangle faces.");
            }

            for (size_t k = 0; k < 3; ++k) {
                triangles[j * 3 + k] = scene->mMeshes[i]->mFaces[j].mIndices[k];
            }
        }

        rtcUnmapBuffer(rtcScene, geomID, RTC_INDEX_BUFFER);

        if (geomID != meshes.size()) {
            rtcUnmapBuffer(rtcScene, geomID, RTC_INDEX_BUFFER);
            rtcDeleteScene(rtcScene);
            throw std::runtime_error("Geometry ID doesn't correspond to mesh index.");
        }

        Mesh mesh;
        mesh.materialID = scene->mMeshes[i]->mMaterialIndex;
        mesh.name = scene->mMeshes[i]->mName.C_Str();
        meshes.push_back(mesh);
    }

    rtcCommit(rtcScene);

    vector<Material> materials;

    for (size_t i = 0; i < scene->mNumMaterials; ++i) {
        Material material;
        
        aiString name;
        scene->mMaterials[i]->Get(AI_MATKEY_NAME, name);
        material.name = name.C_Str();
        
        aiColor3D color(0.0f, 0.0f, 0.0f);
        scene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, color);
        material.diffuse.r = color.r;
        material.diffuse.g = color.g;
        material.diffuse.b = color.b;
        materials.push_back(material);
    }

    Scene result;
    result.rtcScene = rtcScene;
    result.meshes = std::move(meshes);
    result.materials = std::move(materials);
    return result;
}

}
