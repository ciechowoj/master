#include <runtime_assert>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <utility.hpp>
#include <loader.hpp>

#include <iostream>

namespace haste {

using std::move;

template <class Stream> Stream& operator<<(Stream& stream, const aiString& string) {
    return stream << string.C_Str();
}

template <class Stream> Stream& operator<<(Stream& stream, const aiColor3D& color) {
    return stream << "aiColor3D(" << color.r << ", " << color.g << ", " << color.b << ")";
}

template <class Stream> Stream& operator<<(Stream& stream, const aiVector3D& vector) {
    return stream << "aiVector3D(" << vector.x << ", " << vector.y << ", " << vector.z << ")";
}

template <class Stream> Stream& operator<<(Stream& stream, aiLightSourceType type) {
    switch (type) {
        case aiLightSource_UNDEFINED: return stream << "aiLightSource_UNDEFINED";
        case aiLightSource_DIRECTIONAL: return stream << "aiLightSource_DIRECTIONAL";
        case aiLightSource_POINT: return stream << "aiLightSource_POINT";
        case aiLightSource_SPOT: return stream << "aiLightSource_SPOT";
        default: return stream << "undefined";
    }
}

template <class Stream> Stream& operator<<(Stream& stream, const aiCamera& camera) {
    return stream
        << "aiCamera { "
        << "mAspect = " << camera.mAspect << ", "
        << "mClipPlaneFar = " << camera.mClipPlaneFar << ", "
        << "mClipPlaneNear = " << camera.mClipPlaneNear << ", "
        << "mHorizontalFOV = " << camera.mHorizontalFOV << ", "
        << "mLookAt = " << camera.mLookAt << ", "
        << "mName = " << camera.mName << ", "
        << "mPosition = " << camera.mPosition << ", "
        << "mUp = " << camera.mUp << " }";
}

template <class Stream> Stream& operator<<(Stream& stream, const aiLight& light) {
    return stream
        << "aiLight { "
        << "mAngleInnerCone = " << light.mAngleInnerCone << ", "
        << "mAngleOuterCone = " << light.mAngleOuterCone << ", "
        << "mAttenuationConstant = " << light.mAttenuationConstant << ", "
        << "mAttenuationLinear = " << light.mAttenuationLinear << ", "
        << "mAttenuationQuadratic = " << light.mAttenuationQuadratic << ", "
        << "mColorAmbient = " << light.mColorAmbient << ", "
        << "mColorDiffuse = " << light.mColorDiffuse << ", "
        << "mColorSpecular = " << light.mColorSpecular << ", "
        << "mDirection = " << light.mDirection << ", "
        << "mName = " << light.mName << ", "
        << "mPosition = " << light.mPosition << ", "
        << "mType = " << light.mType << " }";
}

template <class Stream> Stream& operator<<(Stream& stream, const aiNode& node) {
    return stream
        << "aiNode { mChildren = {...}, mMeshes = {...}, "
        << "mName = " << node.mName << ", "
        << "mNumChildren = " << node.mNumChildren << ", "
        << "mNumMeshes = " << node.mNumMeshes << ", "
        << "mParent = "
        << (node.mParent ? (const char*)"{ }" : (const char*)"nullptr") << ", "
        << "mTransformation = { } }";
}


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

Mesh aiMeshToMesh(const aiMesh* mesh) {
    runtime_assert(mesh != nullptr);
    runtime_assert(mesh->mNormals != nullptr);
    runtime_assert(mesh->mVertices != nullptr);

    Mesh result;

    if (mesh->mBitangents == nullptr ||
        mesh->mTangents == nullptr) {
        result.indices.resize(mesh->mNumFaces * 3);
        result.bitangents.resize(mesh->mNumFaces * 3);
        result.normals.resize(mesh->mNumFaces * 3);
        result.tangents.resize(mesh->mNumFaces * 3);
        result.vertices.resize(mesh->mNumFaces * 3);

        for (size_t j = 0; j < mesh->mNumFaces; ++j) {
            runtime_assert(mesh->mFaces[j].mNumIndices == 3);

            for (size_t k = 0; k < 3; ++k) {
                unsigned index = mesh->mFaces[j].mIndices[k];
                result.indices[j * 3 + k] = j * 3 + k;
                result.normals[j * 3 + k] = toVec3(mesh->mNormals[index]);
                result.vertices[j * 3 + k] = toVec3(mesh->mVertices[index]);
            }

            vec3 edge = result.vertices[j * 3 + 1] - result.vertices[j * 3 + 0];

            for (size_t k = 0; k < 3; ++k) {
                vec3 normal = result.normals[j * 3 + k];
                vec3 tangent = normalize(edge - dot(normal, edge) * normal);
                vec3 bitangent = normalize(cross(normal, tangent));
                result.bitangents[j * 3 + k] = bitangent;
                result.tangents[j * 3 + k] = tangent;
            }
        }
    }
    else {
        result.bitangents.resize(mesh->mNumVertices);
        result.normals.resize(mesh->mNumVertices);
        result.tangents.resize(mesh->mNumVertices);
        result.vertices.resize(mesh->mNumVertices);

        for (size_t j = 0; j < mesh->mNumVertices; ++j) {
            result.bitangents[j] = toVec3(mesh->mBitangents[j]);
            result.normals[j] = toVec3(mesh->mNormals[j]);
            result.tangents[j] = toVec3(mesh->mTangents[j]);
            result.vertices[j] = toVec3(mesh->mVertices[j]);
        }

        result.indices.resize(mesh->mNumFaces * 3);

        for (size_t j = 0; j < mesh->mNumFaces; ++j) {
            runtime_assert(mesh->mFaces[j].mNumIndices == 3);

            for (size_t k = 0; k < 3; ++k) {
                result.indices[j * 3 + k] = mesh->mFaces[j].mIndices[k];
            }
        }
    }

    result.name = mesh->mName.C_Str();
    result.materialID = mesh->mMaterialIndex;

    return result;
}

Mesh makeMesh(
    const aiScene* scene,
    size_t i) {

    auto mesh = scene->mMeshes[i];

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

    Mesh result;
    result.materialID = scene->mMeshes[i]->mMaterialIndex;
    result.name = scene->mMeshes[i]->mName.C_Str();
    result.indices = std::move(indices);
    result.normals = std::move(normals);
    result.vertices = std::move(vertices);
    return result;
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
            lights.indices[numIndices + j * 3 + k] = scene->mMeshes[i]->mFaces[j].mIndices[k] + numVertices;
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
        lights.exitances[numFaces + j] = emissive(scene->mMaterials[materialID]);
    }
}

shared<Scene> loadScene(string path) {
    Assimp::Importer importer;

    auto flags =
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_PreTransformVertices;

    const aiScene* scene = importer.ReadFile(path, flags);

    using namespace std;

    std::cout << path << std::endl;
    std::cout << "mNumLights: " << scene->mNumLights << std::endl;
    std::cout << "mNumCameras: " << scene->mNumCameras << std::endl;
    std::cout << "mNumNodes: " << scene->mRootNode->mNumChildren << std::endl;

    for (size_t i = 0; i < scene->mNumLights; ++i) {
        std::cout << *scene->mLights[i] << std::endl;
    }

    for (size_t i = 0; i < scene->mNumCameras; ++i) {
        std::cout << *scene->mCameras[i] << std::endl;
    }

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
            meshes.push_back(aiMeshToMesh(scene->mMeshes[i]));
        }
    }

    AreaLights lights;

    for (size_t i = 0; i < emissive.size(); ++i) {
        appendLights(
            lights,
            scene,
            emissive[i]);
    }

    Materials materials;

    for (size_t i = 0; i < scene->mNumMaterials; ++i) {
        materials.names.push_back(name(scene->mMaterials[i]));
        materials.diffuses.push_back(diffuse(scene->mMaterials[i]));
        materials.speculars.push_back(specular(scene->mMaterials[i]));
        materials.bsdfs.push_back(unique<BSDF>(new DiffuseBSDF(materials.diffuses.back())));
    }

    shared<Scene> result = make_shared<Scene>(
        move(materials),
        move(meshes),
        move(lights));

    return result;
}

}
