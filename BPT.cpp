#include <BPT.hpp>
#include <Edge.hpp>

namespace haste {

template <class Beta> BPTBase<Beta>::BPTBase(const shared<const Scene>& scene, size_t minSubpath, float lights, float roulette, float beta, size_t num_threads)
    : Technique(scene, num_threads)
    , _minSubpath(minSubpath)
    , _roulette(roulette)
    , _lights(lights)
{ }

template <class Beta> string BPTBase<Beta>::name() const {
    return Beta::name();
}

template <class Beta>
vec3 BPTBase<Beta>::_traceEye(render_context_t& context, Ray ray) {
    light_path_t light_path;
    vec3 radiance = vec3(0.0f);

    if (_russian_roulette(*context.generator)) {
        return radiance;
    }

    _traceLight(*context.generator, light_path);

    EyeVertex eye[2];
    size_t itr = 0, prv = 1;

    eye[prv].surface = _camera_surface(context);
    eye[prv].omega = -ray.direction;
    eye[prv].throughput = vec3(1.0f) / _roulette;
    eye[prv].specular = 0.0f;
    eye[prv].c = 0;
    eye[prv].C = 0;

    radiance += _connect_eye(context, eye[prv], light_path);

    SurfacePoint surface = _scene->intersect(eye[prv].surface, ray.direction);

    while (surface.is_light()) {
        radiance += eye[prv].throughput * _lights * _scene->queryRadiance(surface, -ray.direction);
        surface = _scene->intersect(surface, ray.direction);
    }

    if (!surface.is_present()) {
        return radiance;
    }

    if (_russian_roulette(*context.generator)) {
        return radiance;
    }

    eye[itr].surface = surface;
    eye[itr].omega = -ray.direction;

    auto edge = Edge(eye[prv], eye[itr]);

    eye[itr].throughput = eye[prv].throughput / _roulette;
    eye[itr].specular = 0.0f;
    eye[itr].c = 1.0f / Beta::beta(edge.fGeometry * _roulette);
    eye[itr].C = 0.0f;

    std::swap(itr, prv);

    while (true) {
        radiance += _connect(eye[prv], light_path);

        auto bsdf = _scene->sampleBSDF(*context.generator, eye[prv].surface, eye[prv].omega);

        while (true) {
            surface = _scene->intersect(surface, bsdf.omega);

            if (!surface.is_present()) {
                return radiance;
            }

            eye[itr].surface = surface;
            eye[itr].omega = -bsdf.omega;

            auto edge = Edge(eye[prv], eye[itr]);

            eye[itr].throughput
                = eye[prv].throughput
                * bsdf.throughput
                * edge.bCosTheta;

            if (l1Norm(eye[itr].throughput) < FLT_EPSILON) {
              return radiance;
            }

            eye[itr].throughput /= bsdf.density;

            eye[prv].specular = max(eye[prv].specular, bsdf.specular);
            eye[itr].specular = bsdf.specular;
            eye[itr].c = 1.0f / Beta::beta(edge.fGeometry * bsdf.density * _roulette);

            eye[itr].C
                = (eye[prv].C
                    * Beta::beta(bsdf.densityRev * _roulette)
                    + eye[prv].c * (1.0f - eye[prv].specular))
                * Beta::beta(edge.bGeometry)
                * eye[itr].c;

            if (surface.is_light()) {
                radiance += _connect_light(eye[itr]);
            }
            else {
                break;
            }
        }

        std::swap(itr, prv);

        if (_russian_roulette(*context.generator)) {
            return radiance;
        }

        eye[prv].throughput /= _roulette;
    }

    return radiance;
}

template <class Beta>
void BPTBase<Beta>::_traceLight(RandomEngine& engine, light_path_t& path) {
    size_t itr = path.size() + 1, prv = path.size();

    if (_russian_roulette(engine)) {
        return;
    }

    LightSample light = _scene->sampleLight(engine);

    path.emplace_back();
    path[prv].surface = light.surface;
    path[prv].omega = path[prv].surface.normal();
    path[prv].throughput = light.radiance() / light.areaDensity() / _roulette;
    path[prv].specular = 0.0f;
    path[prv].a = 1.0f / Beta::beta(light.areaDensity() * _roulette);
    path[prv].A = 0.0f;

    while (!_russian_roulette(engine)) {
        auto bsdf = _scene->sampleBSDF(engine, path[prv].surface, path[prv].omega);

        auto surface = _scene->intersectMesh(path[prv].surface, bsdf.omega);

        if (!surface.is_present()) {
            break;
        }

        path.emplace_back();

        path[itr].surface = surface;
        path[itr].omega = -bsdf.omega;

        auto edge = Edge(path[prv], path[itr]);

        path[itr].throughput
            = path[prv].throughput
            * bsdf.throughput
            * edge.bCosTheta
            / _roulette;

        if (l1Norm(path[itr].throughput) < FLT_EPSILON) {
            path.pop_back();
            break;
        }

        path[itr].throughput /= bsdf.density;

        path[prv].specular = max(path[prv].specular, bsdf.specular * _roulette);
        path[itr].specular = bsdf.specular;

        path[itr].a = 1.0f / Beta::beta(edge.fGeometry * bsdf.density * _roulette);

        path[itr].A
            = (path[prv].A
                * Beta::beta(bsdf.densityRev * _roulette)
                + path[prv].a * (1.0f - path[prv].specular))
            * Beta::beta(edge.bGeometry)
            * path[itr].a;

        if (bsdf.specular == 1.0f) {
            path[prv] = path[itr];
            path.pop_back();
        }
        else {
            prv = itr;
            ++itr;
        }
    }

    auto bsdf = _scene->sampleBSDF(
        engine,
        path[prv].surface,
        path[prv].omega);

    if (bsdf.specular == 1.0f) {
        path.pop_back();
    }
}

template <class Beta> vec3 BPTBase<Beta>::_connect(
    const EyeVertex& eye,
    const LightVertex& light) {
    vec3 omega = normalize(eye.surface.position() - light.surface.position());

    auto lightBSDF = _scene->queryBSDF(light.surface, light.omega, omega);
    auto eyeBSDF = _scene->queryBSDF(eye.surface, -omega, eye.omega);

    auto edge = Edge(light, eye, omega);

    float Ap
        = (light.A * Beta::beta(lightBSDF.densityRev * _roulette) + light.a * (1.0f - light.specular))
        * Beta::beta(edge.bGeometry * eyeBSDF.densityRev * _roulette);

    float Cp
        = (eye.C * Beta::beta(eyeBSDF.density * _roulette) + eye.c * (1.0f - eye.specular))
        * Beta::beta(edge.fGeometry * lightBSDF.density * _roulette);

    float weightInv = Ap + Cp + 1.0f;

    vec3 result = _scene->occluded(eye.surface, light.surface)
        * light.throughput
        * lightBSDF.throughput
        * eye.throughput
        * eyeBSDF.throughput
        * edge.bCosTheta
        * edge.fGeometry;

    return l1Norm(result) < FLT_EPSILON ? vec3(0.0f) : result / weightInv;
}

template <class Beta> vec3 BPTBase<Beta>::_connect_light(const EyeVertex& eye) {
    if (!eye.surface.is_light()) {
        return vec3(0.0f);
    }

    auto lsdf = _scene->queryLSDF(eye.surface, eye.omega);

    float Cp
        = (eye.C * Beta::beta(lsdf.omegaDensity() * _roulette) + eye.c * (1.0f - eye.specular))
        * Beta::beta(lsdf.areaDensity() * _roulette);

    float weightInv = Cp + 1.0f;

    return lsdf.radiance()
        * eye.throughput
        / weightInv;
}

template <class Beta>
vec3 BPTBase<Beta>::_connect(const EyeVertex& eye, const light_path_t& path) {
    vec3 radiance = vec3(0.0f);

    for (size_t i = 0; i < path.size(); ++i) {
        radiance += _connect(eye, path[i]);
    }

    return radiance;
}

template <class Beta>
vec3 BPTBase<Beta>::_connect_eye(
    render_context_t& context,
    const EyeVertex& eye,
    const light_path_t& path) {
    vec3 radiance = vec3(0.0f);

    for (size_t i = 1; i < path.size(); ++i) {
        vec3 omega = normalize(path[i].surface.position() - eye.surface.position());

        radiance += _accumulate(
            context,
            omega,
            [&] {
                float correct_normal = abs(dot(omega, path[i].surface.gnormal)
                    / dot(omega, path[i].surface.normal()));

                return _connect(eye, path[i]) * context.focal_factor_y * correct_normal;
            });
    }

    return radiance;
}

template <class Beta>
bool BPTBase<Beta>::_russian_roulette(random_generator_t& generator) const {
    return _roulette < generator.sample();
}

BPTb::BPTb(const shared<const Scene>& scene, size_t minSubpath, float lights, float roulette, float beta, size_t num_threads)
    : BPTBase<VariableBeta>(scene, minSubpath, lights, roulette, beta, num_threads)
{
    VariableBeta::init(beta);
}

template class BPTBase<FixedBeta<0>>;
template class BPTBase<FixedBeta<1>>;
template class BPTBase<FixedBeta<2>>;
template class BPTBase<VariableBeta>;

}
