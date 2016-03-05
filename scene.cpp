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

tinyobj::scene_t tinyobj::load(const std::string& obj) {
    std::string errors;

    scene_t scene;

    bool result = tinyobj::LoadObj(
        scene.shapes,
        scene.materials,
        errors,
        obj.c_str(),
        (dirname(obj) + "/").c_str()
        );

    if (!errors.empty()) {
        std::cerr << errors << std::endl;
    }

    if (!result) {
        throw std::runtime_error(errors);
    }

    return scene;
}

std::ostream& operator<<(
    std::ostream& stream,
    const obj::scene_t& scene) {
    return stream << "scene_t(shapes = [...], materials = [...])";
}

std::ostream& operator<<(
    std::ostream& stream,
    const tinyobj::shape_t& shape) {
    stream << "shape_t(name = " << shape.name << ", mesh = mesh_t(...))";
    return stream;
}

std::ostream& operator<<(
    std::ostream& stream,
    const obj::mesh_t& shape) {
    return stream << "mesh_t(positions = [...], normals = [...], texcoords = [...], "
        << "indices = [...], num_vertices = [...], material_ids = [...], tags = [...])";
}

std::ostream& operator<<(
    std::ostream& stream,
    const std::vector<tinyobj::shape_t>& shapes) {
    stream << "[";

    if (shapes.size() != 0) {
        for (size_t i = 0; i < shapes.size() - 1; ++i) {
            stream << shapes[i] << ", ";
        }
        
        stream << shapes[shapes.size() - 1];
    }

    stream << "]";

    return stream;
}




aabb_t aabb(const scene_t& scene) {
    aabb_t result;
    result.a = vec3(INFINITY);
    result.b = vec3(-INFINITY);

    const size_t num_shapes = scene.shapes.size();

    for (size_t shape = 0; shape < num_shapes; ++shape) {
        const auto& positions = scene.shapes[shape].mesh.positions;
        const size_t num_positions = positions.size();

        for (size_t position = 0; position < num_positions; position += 3) {
            result.a.x = min(result.a.x, positions[position + 0]);
            result.a.y = min(result.a.y, positions[position + 1]);
            result.a.z = min(result.a.z, positions[position + 2]);
            result.b.x = max(result.b.x, positions[position + 0]);
            result.b.y = max(result.b.y, positions[position + 1]);
            result.b.z = max(result.b.z, positions[position + 2]);
        }
    }

    return result;
}

std::ostream& operator<<(
    std::ostream& stream,
    const aabb_t& aabb)
{
    return stream << "aabb_t(a = " << aabb.a << ", b = " << aabb.b << ")" << std::endl;
}


namespace haste {

Scene loadScene(RTCDevice device, string path) {
    Assimp::Importer importer;

    auto flags =
        aiProcess_CalcTangentSpace |
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

    vector<Material> materials;

    std::cout << "numMaterials: " << scene->mNumMaterials << std::endl;

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
