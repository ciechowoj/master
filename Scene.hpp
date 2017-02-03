#pragma once
#include <Intersector.hpp>
#include <Prerequisites.hpp>

#include <AreaLights.hpp>
#include <BSDF.hpp>
#include <Cameras.hpp>
#include <Materials.hpp>

#include <SurfacePoint.hpp>

namespace haste {

using namespace glm;

template <class T>
using shared = std::shared_ptr<T>;
using std::make_shared;

using std::vector;
using std::string;
using std::size_t;
using std::move;

struct Ray;

class Scene : public Intersector {
 public:
  Scene(Cameras&& cameras, Materials&& materials, vector<Mesh>&& meshes,
        AreaLights&& areaLights);

  Cameras _cameras;
  const vector<Mesh> meshes;
  AreaLights lights;
  const Materials materials;

  const Cameras& cameras() const { return _cameras; }

  void buildAccelStructs(RTCDevice device);

  const BSDF& queryBSDF(const SurfacePoint& surface) const;

  SurfacePoint querySurface(const RayIsect& isect) const;

  const LSDFQuery queryLSDF(const SurfacePoint& surface,
                            const vec3& omega) const;

  using Intersector::intersect;

  float occluded(const SurfacePoint& origin,
                 const SurfacePoint& target) const override;

  virtual SurfacePoint intersect(const SurfacePoint& surface, vec3 direction,
                                 float tfar) const override;

  virtual SurfacePoint intersectMesh(const SurfacePoint& surface,
                                     vec3 direction, float tfar) const override;

  using Intersector::intersectMesh;

  const size_t numNormalRays() const;
  const size_t numShadowRays() const;
  const size_t numRays() const;

  const LightSample sampleLight(RandomEngine& engine) const;

  const BSDFSample sampleBSDF(RandomEngine& engine, const SurfacePoint& surface,
                              const vec3& omega) const;

  const BSDFQuery queryBSDF(const SurfacePoint& surface, const vec3& incident,
                            const vec3& outgoing) const;

 private:
  int32_t _material_id_to_light_id(int32_t) const;
  int32_t _light_id_to_material_id(int32_t) const;
};
}
