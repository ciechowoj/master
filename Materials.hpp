#pragma once
#include <memory>
#include <glm>
#include <utility.hpp>
#include <BSDF.hpp>

namespace haste {

template <class T> using unique = std::unique_ptr<T>;

struct SurfacePoint;
struct Photon;

enum BSDFType {
    BDSFType
};

using Material = size_t;

class Materials {
public:
    vector<string> names;
    vector<unique<BSDF>> bsdfs;

    size_t numMaterials() const {
    	return names.size();
    }

    const string& name(size_t index) const {
    	return names[index];
    }

    const BSDF& queryBSDF(Material material) const;
};

}
