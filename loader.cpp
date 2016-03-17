#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <loader.hpp>

namespace haste {

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
    const aiScene* scene,
    size_t i,
    size_t numMeshes) {

    vector<vec3> vertices;

    for (size_t j = 0; j < scene->mMeshes[i]->mNumVertices; ++j) {
        vertices.push_back(vec3(0));
        vertices[j].x = scene->mMeshes[i]->mVertices[j].x;
        vertices[j].y = scene->mMeshes[i]->mVertices[j].y;
        vertices[j].z = scene->mMeshes[i]->mVertices[j].z;
    }

    vector<int> indices(scene->mMeshes[i]->mNumFaces * 3);

    for (size_t j = 0; j < scene->mMeshes[i]->mNumFaces; ++j) {
        if (scene->mMeshes[i]->mFaces[j].mNumIndices != 3) {
            throw std::runtime_error("Loaded scene contains non triangle faces.");
        }

        for (size_t k = 0; k < 3; ++k) {
            indices[j * 3 + k] = scene->mMeshes[i]->mFaces[j].mIndices[k];
        }
    }

    if (scene->mMeshes[i]->mNormals == nullptr) {
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
    mesh.vertices = std::move(vertices);
    return mesh;
}

Scene loadScene(string path) {
    Assimp::Importer importer;

    auto flags =
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices;

    const aiScene* scene = importer.ReadFile(path, flags);

    if (!scene) {
        throw std::runtime_error("Cannot load \"" + path + "\" scene.");
    }

    vector<Mesh> meshes;
    vector<size_t> emissive;

    for (size_t i = 0; i < scene->mNumMeshes; ++i) {
        if (isEmissive(scene, i)) {
            emissive.push_back(i);
        }
        else {
            meshes.push_back(makeMesh(scene, i, meshes.size()));
        }
    }

    vector<Mesh> areaLights;

    for (size_t i = 0; i < emissive.size(); ++i) {
        areaLights.push_back(makeMesh(scene, emissive[i], meshes.size() + areaLights.size()));
    }

    vector<Material> materials;

    for (size_t i = 0; i < scene->mNumMaterials; ++i) {
        Material material;
        material.name = name(scene->mMaterials[i]);
        material.ambient = ambient(scene->mMaterials[i]);
        material.diffuse = diffuse(scene->mMaterials[i]);

        materials.push_back(material);
    }

    Scene result;
    result.meshes = std::move(meshes);
    result.areaLights = std::move(areaLights);
    result.materials = std::move(materials);
    return result;
}





}
