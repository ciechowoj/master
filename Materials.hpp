#pragma once
#include <memory>
#include <glm>
#include <utility.hpp>
#include <BSDF.hpp>

namespace haste {

template <class T> using unique = std::unique_ptr<T>;

struct SurfacePoint;

class Materials {
public:
    int32_t lights_offset = 0;
    vector<string> names;
    vector<unique<BSDF>> bsdfs;

    size_t numMaterials() const {
    	return names.size();
    }

    const string& name(size_t index) const {
    	return names[index];
    }
};

}
