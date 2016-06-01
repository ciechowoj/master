#include <runtime_assert>
#include <cstring>
#include <GLFW/glfw3.h>
#include <PhotonMapping.hpp>

namespace haste {

PhotonMapping::PhotonMapping(
    size_t numPhotons,
    size_t numNearest,
    float maxDistance)
    : _numPhotons(numPhotons)
    , _numPhotonsInv(1.f / numPhotons)
    , _numNearest(numNearest)
    , _maxDistance(maxDistance) {
    runtime_assert(numNearest < _maxNumNearest);
}

void PhotonMapping::preprocess(
    const shared<const Scene>& scene,
    RandomEngine& engine,
    const function<void(string, float)>& progress,
    bool parallel)
{
    Technique::preprocess(scene, engine, progress, parallel);

    _totalPower = _scene->lights.totalPower();

    if (_auxiliary.empty()) {
        _numEmitted = 0;
    }

    const size_t batchSize = 1000;
    double startTime = glfwGetTime();

    while (_numEmitted < _numPhotons) {
        const size_t begin = _numEmitted;
        const size_t end = min(_numPhotons, begin + batchSize);
        const size_t stored = _auxiliary.size();

        _scatterPhotons(engine, begin, end);
        _numEmitted = end;

        double time = glfwGetTime();
        if (time - startTime < 0.033f) {
            progress("Scattering photons", float(_numEmitted) / float(_numPhotons));
            startTime = time;
        }
    }

    progress("Building photon map", 0.0f);
    _buildPhotonMap();
    progress("Building photon map", 1.0f);

}

void PhotonMapping::render(
    ImageView& view,
    RandomEngine& engine,
    size_t cameraId)
{
    auto trace = [&](RandomEngine& engine, Ray ray) -> vec3 {
        return _gather(engine, ray);
    };

    for_each_ray(view, engine, _scene->cameras(), cameraId, trace);
}

void PhotonMapping::_renderPhotons(
    ImageView& view,
    size_t cameraId,
    size_t begin,
    size_t end)
{
    size_t width = view.width();
    size_t height = view.height();
    float f5width = 0.5f * float(width);
    float f5height = 0.5f * float(height);
    auto& cameras = _scene->cameras();

    mat4 viewMatrix = inverse(cameras.view(cameraId));
    mat4 proj = cameras.proj(cameraId, float(width) / height) * viewMatrix;

    const float scaleFactor = 1.f / (_totalPower * _numPhotonsInv);

    for (size_t i = begin; i < end; ++i) {
        vec4 h = proj * vec4(_auxiliary[i].position, 1.0);
        vec3 c = _auxiliary[i].power * scaleFactor;
        vec3 v = h.xyz() / h.w;

        if (-1.0f <= v.z && v.z <= +1.0f) {
            int x = int((v.x + 1.0f) * f5width + 0.5f);
            int y = int((v.y + 1.0f) * f5height + 0.5f);

            for (int j = y - 0; j <= y + 0; ++j) {
                for (int i = x - 0; i <= x + 0; ++i) {
                    if (view.inWindow(i, j)) {
                        view.absAt(i, j) += vec4(c, 1.0f);
                        view.absAt(i, j).w = 1.0f;
                    }
                }
            }
        }
    }
}

string PhotonMapping::name() const {
    return "Photon Mapping";
}

void PhotonMapping::_scatterPhotons(RandomEngine& engine, size_t begin, size_t end) {
    const float scaleFactor = _totalPower * _numPhotonsInv;

    for (size_t i = begin; i < end; ++i) {
        Photon photon = _scene->lights.emit(engine);
        photon.power *= scaleFactor;

        for (size_t j = 0; ; ++j) {
            RayIsect isect = _scene->intersect(
                photon.position,
                photon.direction);

            if (!isect.isPresent() || !isect.isMesh()) {
                break;
            }

            photon.position = isect.position();
            photon.direction = -photon.direction;

            SurfacePoint point = _scene->querySurface(isect);
            auto sample = _scene->queryBSDF(isect).scatter(
                engine,
                point,
                photon.direction);

            if (!sample.specular()) {
                _auxiliary.push_back(photon);
            }

            if (!sample.zero()) {
                photon.power *= sample.throughput();
                photon.direction = sample.omega();
            }
            else {
                break;
            }
        }
    }
}

void PhotonMapping::_buildPhotonMap() {
    _photons = KDTree3D<Photon>(std::move(_auxiliary));
    _auxiliary = vector<Photon>();
}

vec3 PhotonMapping::_gather(RandomEngine& source, Ray ray) {
    Photon auxiliary[_maxNumNearest];
    vec3 radiance = vec3(0.0f);

    auto isect = _scene->intersect(ray.origin, ray.direction);

    while (isect.isLight()) {
        radiance += _scene->queryRadiance(isect);

        ray.origin = isect.position();
        isect = _scene->intersect(ray);
    }

    while (isect.isPresent() && !isect.isMesh()) {
        ray.origin = isect.position();
        isect = _scene->intersect(ray.origin, ray.direction);
    }

    if (isect.isPresent()) {
        SurfacePoint point = _scene->querySurface(isect);

        size_t queried = _photons.query_k(
            auxiliary,
            isect.position(),
            _numNearest,
            _maxDistance);

        if (queried > 8) {
            auto& bsdf = _scene->queryBSDF(isect);
            vec3 result = vec3(0.0f);

            float radius2 = 0.0;

            for (size_t i = 0; i < queried; ++i) {
                result += auxiliary[i].power *
                    bsdf.query(
                        point,
                        auxiliary[i].direction,
                        -normalize(ray.direction),
                        point.normal());

                radius2 = max(radius2, distance2(isect.position(), auxiliary[i].position));
            }

            radiance += result / (radius2 * pi<float>());
        }
    }

    return radiance;
}

}
