#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <utility.hpp>
#include <loader.hpp>

namespace haste {

using std::move;

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
    size_t i) {

    vector<vec3> vertices;

    for (size_t j = 0; j < scene->mMeshes[i]->mNumVertices; ++j) {
        vertices.push_back(vec3(0));
        vertices[j] = toVec3(scene->mMeshes[i]->mVertices[j]);
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

void appendLights(
    AreaLights& lights,
    const aiScene* scene,
    size_t i) {

    lights.names.push_back(scene->mMeshes[i]->mName.C_Str());
    size_t numIndices = lights.indices.size();
    lights.offsets.push_back(numIndices);

    size_t numVertices = lights.vertices.size();
    lights.vertices.resize(numVertices + scene->mMeshes[i]->mNumVertices);

    for (size_t j = 0; j < scene->mMeshes[i]->mNumVertices; ++j) {
        lights.vertices.push_back(vec3(0));
        lights.vertices[numVertices + j].x = scene->mMeshes[i]->mVertices[j].x;
        lights.vertices[numVertices + j].y = scene->mMeshes[i]->mVertices[j].y;
        lights.vertices[numVertices + j].z = scene->mMeshes[i]->mVertices[j].z;
    }

    lights.indices.resize(numIndices + scene->mMeshes[i]->mNumFaces * 3);

    for (size_t j = 0; j < scene->mMeshes[i]->mNumFaces; ++j) {
        if (scene->mMeshes[i]->mFaces[j].mNumIndices != 3) {
            throw std::runtime_error("Loaded scene contains non triangle faces.");
        }

        for (size_t k = 0; k < 3; ++k) {
            lights.indices[numIndices + j * 3 + k] = scene->mMeshes[i]->mFaces[j].mIndices[k];
        }
    }

    if (scene->mMeshes[i]->mNormals == nullptr) {
        throw std::runtime_error("Normal vectors are not present.");
    }

    lights.toWorldMs.resize(lights.vertices.size());

    if (scene->mMeshes[i]->mTangents != nullptr &&
        scene->mMeshes[i]->mBitangents != nullptr) {
        for (size_t j = 0; j < scene->mMeshes[i]->mNumVertices; ++j) {
            size_t k = numVertices + j;
            lights.toWorldMs[k][0] = toVec3(scene->mMeshes[i]->mBitangents[j]);
            lights.toWorldMs[k][1] = toVec3(scene->mMeshes[i]->mNormals[j]);
            lights.toWorldMs[k][2] = toVec3(scene->mMeshes[i]->mTangents[j]);
        }
    }
    else {
        for (size_t j = 0; j < scene->mMeshes[i]->mNumVertices; ++j) {
            size_t k = numVertices + j;
            vec3 normal = toVec3(scene->mMeshes[i]->mNormals[j]);
            vec3 tangent = lights.vertices[k + 1] - lights.vertices[k + 0];
            tangent = tangent - dot(normal, tangent) * normal;
            vec3 bitangent = cross(normal, tangent);

            lights.toWorldMs[k][0] = normalize(bitangent);
            lights.toWorldMs[k][1] = normal;
            lights.toWorldMs[k][2] = normalize(tangent);
        }
    }

    size_t numFaces = numIndices / 3;
    lights.exitances.resize(lights.indices.size() / 3);

    for (size_t j = 0; j < scene->mMeshes[i]->mNumFaces; ++j) {
        auto materialID = scene->mMeshes[i]->mMaterialIndex;
        vec3 power = emissive(scene->mMaterials[materialID]);
        lights.exitances[numFaces + j] = power / lights.faceArea(numFaces + j);
    }
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
            meshes.push_back(makeMesh(scene, i));
        }
    }

    AreaLights areaLights;

    for (size_t i = 0; i < emissive.size(); ++i) {
        appendLights(
            areaLights, 
            scene, 
            emissive[i]);
    }

    Materials materials;

    for (size_t i = 0; i < scene->mNumMaterials; ++i) {
        materials.names.push_back(name(scene->mMaterials[i]));
        materials.diffuses.push_back(diffuse(scene->mMaterials[i]));
        materials.speculars.push_back(specular(scene->mMaterials[i]));
        materials.bsdfs.push_back(BSDF::lambert(materials.diffuses.back()));
    }
    
    Scene result = Scene(
        move(materials),
        move(meshes), 
        move(areaLights));

    return result;
}

}
